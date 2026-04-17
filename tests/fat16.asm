; fat16.asm - FAT16 filesystem reader, 32-bit protected mode

[bits 32]
section .text

extern ata_read_sectors

global fat16_init
global fat16_open
global fat16_read_file

; Internal symbols exported for fs_manager.asm
global _part_start
global _cwd_cluster
global _bytes_per_sec
global _sec_per_clus
global _fat_lba
global _fat_size
global _root_lba
global _root_entries
global _data_lba
global _fat_cached_sec
global _sector_buf
global _fat_cache
global _mkdir_sector
global _clus_buf
global _file_entry_size
global _open_file_size
global _fat16_next_cluster
global _fat_write_entry
global _alloc_cluster
global _add_file_entry

BPB_BytsPerSec  equ  11
BPB_SecPerClus  equ  13
BPB_RsvdSecCnt  equ  14
BPB_NumFATs     equ  16
BPB_RootEntCnt  equ  17
BPB_FATSz16     equ  22

DIR_Name        equ   0
DIR_Attr        equ  11
DIR_FstClusLO   equ  26
DIR_FileSize    equ  28

ATTR_VOLUME_ID  equ 0x08
ATTR_LFN        equ 0x0F

FAT16_EOC       equ 0xFFF8

section .bss
_part_start:     resd 1
_bytes_per_sec:  resd 1
_sec_per_clus:   resd 1
_fat_lba:        resd 1
_fat_size:       resd 1
_root_lba:       resd 1
_root_entries:   resd 1
_data_lba:       resd 1
_fat_cached_sec: resd 1
_open_file_size: resd 1
_sector_buf:     resb 512
_fat_cache:      resb 512
_cwd_cluster:    resd 1      ; 0 = root directory
_cwd_path:       resb 256
_cwd_path_len:   resd 1
_open_lba:       resd 1
_open_clus:      resd 1
_open_srem:      resd 1

section .text

; fat16_init([esp+4] = partition_lba)
fat16_init:
    push ebp
    mov  ebp, esp
    push ebx

    mov  eax, [ebp+8]
    mov  [_part_start], eax
    mov  dword [_fat_cached_sec], 0xFFFFFFFF
    mov  byte [_cwd_path],     '/'
    mov  byte [_cwd_path + 1], 0
    mov  dword [_cwd_path_len], 1

    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    movzx eax, word [_sector_buf + BPB_BytsPerSec]
    mov   [_bytes_per_sec], eax

    movzx eax, byte [_sector_buf + BPB_SecPerClus]
    mov   [_sec_per_clus], eax

    movzx eax, word [_sector_buf + BPB_RsvdSecCnt]
    add   eax, [_part_start]
    mov   [_fat_lba], eax

    movzx eax, word [_sector_buf + BPB_FATSz16]
    mov   [_fat_size], eax

    movzx ecx, byte [_sector_buf + BPB_NumFATs]
    imul  ecx, [_fat_size]
    mov   eax, [_fat_lba]
    add   eax, ecx
    mov   [_root_lba], eax

    movzx eax, word [_sector_buf + BPB_RootEntCnt]
    mov   [_root_entries], eax

    mov   eax, [_root_entries]
    shl   eax, 5
    xor   edx, edx
    div   dword [_bytes_per_sec]
    add   eax, [_root_lba]
    mov   [_data_lba], eax

    pop  ebx
    pop  ebp
    ret

; fat16_open([esp+4] = 11-byte name ptr)
; Searches CWD. Returns EAX = first cluster, 0 if not found.
fat16_open:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]
    mov  ebx, [_cwd_cluster]
    cmp  ebx, 0
    je   .search_root

.subdir_cluster_loop:
    cmp  ebx, 2
    jb   .not_found
    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]
    mov  [_open_lba],  eax
    mov  [_open_clus], ebx
    mov  eax, [_sec_per_clus]
    mov  [_open_srem], eax
.subdir_sec_loop:
    mov  eax, [_open_lba]
    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    mov  edi, _sector_buf
    call _open_scan_sector
    test eax, eax
    jnz  .done
    mov  eax, [_open_lba]
    inc  eax
    mov  [_open_lba], eax
    dec  dword [_open_srem]
    jnz  .subdir_sec_loop
    mov  ebx, [_open_clus]
    call _fat16_next_cluster
    mov  ebx, eax
    cmp  ebx, FAT16_EOC
    jae  .not_found
    cmp  ebx, 0
    je   .not_found
    jmp  .subdir_cluster_loop

.search_root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  [_open_srem], eax
    mov  eax, [_root_lba]
    mov  [_open_lba],  eax
.root_sec_loop:
    mov  eax, [_open_lba]
    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    mov  edi, _sector_buf
    call _open_scan_sector
    test eax, eax
    jnz  .done
    mov  eax, [_open_lba]
    inc  eax
    mov  [_open_lba], eax
    dec  dword [_open_srem]
    jnz  .root_sec_loop

.not_found:
    xor  eax, eax
.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

_open_scan_sector:
    push ecx
    push esi
    push edi
    mov  ecx, 16
.oloop:
    cmp  byte [edi], 0x00
    je   .onf
    cmp  byte [edi], 0xE5
    je   .onext
    movzx eax, byte [edi + DIR_Attr]
    cmp  al, ATTR_LFN
    je   .onext
    test al, ATTR_VOLUME_ID
    jnz  .onext
    push ecx
    push edi
    push esi
    mov  ecx, 11
    repe cmpsb
    pop  esi
    pop  edi
    pop  ecx
    jne  .onext
    mov  ecx, [edi + DIR_FileSize]
    mov  [_open_file_size], ecx
    movzx eax, word [edi + DIR_FstClusLO]
    pop  edi
    pop  esi
    pop  ecx
    ret
.onext:
    add  edi, 32
    loop .oloop
.onf:
    xor  eax, eax
    pop  edi
    pop  esi
    pop  ecx
    ret

; fat16_read_file([esp+4] = first_cluster, [esp+8] = dest_buf)
; Returns EAX = bytes read
fat16_read_file:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  ebx, [ebp+8]
    mov  edi, [ebp+12]
    xor  esi, esi

.cluster_loop:
    cmp  ebx, 2
    jb   .done

    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    push edi
    push dword [_sec_per_clus]
    push eax
    call ata_read_sectors
    add  esp, 12

    mov  eax, [_sec_per_clus]
    shl  eax, 9
    add  edi, eax
    add  esi, eax

    call _fat16_next_cluster
    mov  ebx, eax

    cmp  ebx, FAT16_EOC
    jae  .done
    cmp  ebx, 0xFFF7
    je   .done
    cmp  ebx, 0
    je   .done

    jmp  .cluster_loop

.done:
    mov  eax, esi
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; _fat16_next_cluster - EBX = cluster in, EAX = next cluster out
_fat16_next_cluster:
    push ebx
    push edi

    mov  eax, ebx
    shl  eax, 1
    xor  edx, edx
    div  dword [_bytes_per_sec]     ; eax = FAT sector index, edx = byte offset in sector
    add  eax, [_fat_lba]

    cmp  eax, [_fat_cached_sec]
    je   .cached

    mov  [_fat_cached_sec], eax
    push edx                        ; save byte offset — ata_read_sectors clobbers edx via port I/O
    push dword _fat_cache
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    pop  edx                        ; restore byte offset

.cached:
    movzx eax, word [_fat_cache + edx]

    pop  edi
    pop  ebx
    ret

; ============================================================================
; fat16_list_dir
;
; Walks the root directory and calls a user-supplied callback for every real
; file entry found (volume labels and LFN entries are skipped).
;
; Usage:
;   push dword <callback_ptr>
;   call fat16_list_dir
;   add  esp, 4
;
; The callback is called cdecl for each file:
;   push dword <file_size>      ; [esp+8] inside callback
;   push dword <first_cluster>  ; [esp+4] inside callback  (not yet used)
;   push dword <name_ptr>       ; ptr to 11-byte 8.3 name in our sector buffer
;   call <callback>
;   add  esp, 12
;
; The callback must NOT modify the sector buffer. It should copy the name
; out if it needs to keep it past the call.
;
; The 11-byte name pointer is inside _sector_buf and is only valid during
; the callback. name[0..7] = base name (space-padded), name[8..10] = ext.
; ============================================================================

global fat16_list_dir

fat16_list_dir:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]           ; callback pointer

    ; compute number of root dir sectors
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  ebx, eax               ; ebx = sectors remaining

    mov  eax, [_root_lba]       ; eax = current LBA

.sector_loop:
    push ebx
    push eax
    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    pop  eax
    pop  ebx

    mov  edi, _sector_buf
    mov  ecx, 16                ; 16 entries per 512-byte sector

.entry_loop:
    ; end of directory
    cmp  byte [edi], 0x00
    je   .done

    ; deleted entry
    cmp  byte [edi], 0xE5
    je   .next_entry

    ; skip LFN entries
    movzx edx, byte [edi + DIR_Attr]
    cmp  dl, ATTR_LFN
    je   .next_entry

    ; skip volume label
    test dl, ATTR_VOLUME_ID
    jnz  .next_entry

    ; it's a real entry — call the callback
    ; args: name_ptr, first_cluster, file_size
    push ecx                            ; save entry loop counter
    push esi                            ; save callback pointer

    push dword [edi + DIR_FileSize]     ; arg3: file size
    movzx eax, word [edi + DIR_FstClusLO]
    push eax                            ; arg2: first cluster
    push edi                            ; arg1: pointer to 11-byte name

    call esi                            ; call the callback
    add  esp, 12

    pop  esi                            ; restore callback pointer
    pop  ecx                            ; restore entry loop counter

.next_entry:
    add  edi, 32
    loop .entry_loop

    inc  eax
    dec  ebx
    jnz  .sector_loop

.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ============================================================================
; Extended FAT16 — subdirectory and CWD support
; ============================================================================

ATTR_DIRECTORY  equ 0x10

section .text

global fat16_get_cwd_cluster
global fat16_get_cwd_path
global fat16_get_cwd_path_len
global fat16_set_cwd_path_len
global fat16_cd_up
global fat16_set_cwd_cluster
global fat16_list_dir_cluster
global fat16_cd

; ----------------------------------------------------------------------------
; fat16_get_cwd_cluster - returns current working directory cluster in EAX
; ----------------------------------------------------------------------------
fat16_get_cwd_cluster:
    mov  eax, [_cwd_cluster]
    ret

fat16_get_cwd_path:
    mov  eax, _cwd_path
    ret

fat16_get_cwd_path_len:
    mov  eax, [_cwd_path_len]
    ret

fat16_set_cwd_path_len:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp+8]
    mov  [_cwd_path_len], eax
    pop  ebp
    ret

fat16_set_cwd_cluster:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp+8]
    mov  [_cwd_cluster], eax
    pop  ebp
    ret

fat16_cd_up:
    push ebx
    push esi
    push edi
    cmp  dword [_cwd_cluster], 0
    je   .go_root
    mov  ebx, [_cwd_cluster]
    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]
    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    movzx eax, word [_sector_buf + 32 + DIR_FstClusLO]
    mov  [_cwd_cluster], eax
    mov  edi, _cwd_path
    mov  ecx, [_cwd_path_len]
    lea  esi, [edi + ecx - 1]
.scan:
    cmp  esi, edi
    je   .at_root
    cmp  byte [esi], '/'
    je   .found_slash
    dec  esi
    jmp  .scan
.found_slash:
    cmp  esi, edi
    je   .at_root
    mov  byte [esi], 0
    mov  ecx, esi
    sub  ecx, edi
    mov  [_cwd_path_len], ecx
    jmp  .cdup_done
.at_root:
.go_root:
    mov  dword [_cwd_cluster], 0
    mov  byte [_cwd_path],     '/'
    mov  byte [_cwd_path + 1], 0
    mov  dword [_cwd_path_len], 1
.cdup_done:
    mov  eax, 1
    pop  edi
    pop  esi
    pop  ebx
    ret


; ----------------------------------------------------------------------------
; fat16_list_dir_cluster
;
;   push dword <callback_ptr>
;   push dword <cluster>        ; 0 = root
;   call fat16_list_dir_cluster
;   add  esp, 8
;
; Calls callback(name_ptr, first_cluster, file_size) for every entry.
; Skips volume labels, LFN entries, and the . / .. entries.
; ----------------------------------------------------------------------------
fat16_list_dir_cluster:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  ebx, [ebp+8]           ; cluster (0 = root)
    mov  esi, [ebp+12]          ; callback

    cmp  ebx, 0
    je   .root

    ; ── subdirectory: iterate cluster chain ─────────────────────────────────
.subdir_cluster_loop:
    cmp  ebx, 2
    jb   .done

    ; LBA of this cluster
    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    ; read each sector in the cluster
    mov  edi, [_sec_per_clus]
.subdir_sec_loop:
    push edi
    push ebx
    push eax

    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ebx
    pop  edi

    push eax
    push ebx
    push edi
    push esi
    mov  edi, _sector_buf
    mov  ecx, 16
    call _scan_entries          ; scan 16 entries, callback in esi
    pop  esi
    pop  edi
    pop  ebx
    pop  eax

    inc  eax
    dec  edi
    jnz  .subdir_sec_loop

    ; next cluster
    call _fat16_next_cluster    ; ebx in, eax out
    mov  ebx, eax
    cmp  ebx, FAT16_EOC
    jae  .done
    cmp  ebx, 0
    je   .done
    jmp  .subdir_cluster_loop

    ; ── root directory: fixed region ────────────────────────────────────────
.root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  ebx, eax               ; root dir sector count

    mov  eax, [_root_lba]

.root_sector_loop:
    push ebx
    push eax
    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12
    pop  eax
    pop  ebx

    push eax
    push ebx
    push esi
    mov  edi, _sector_buf
    mov  ecx, 16
    call _scan_entries
    pop  esi
    pop  ebx
    pop  eax

    inc  eax
    dec  ebx
    jnz  .root_sector_loop

.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _scan_entries (internal)
;   edi = pointer to first 32-byte entry in sector
;   ecx = number of entries to scan (16)
;   esi = callback pointer
; Calls callback(name_ptr, first_cluster, file_size) for each real entry.
; Skips deleted, LFN, volume label, and dot entries.
; ----------------------------------------------------------------------------
_scan_entries:
    push ebp
    mov  ebp, esp
    push ebx

.loop:
    cmp  ecx, 0
    je   .done

    cmp  byte [edi], 0x00       ; end of directory
    je   .done

    cmp  byte [edi], 0xE5       ; deleted
    je   .next

    movzx edx, byte [edi + DIR_Attr]
    cmp  dl, ATTR_LFN
    je   .next
    test dl, ATTR_VOLUME_ID
    jnz  .next

    ; skip . and .. entries
    cmp  byte [edi], '.'
    je   .next

    ; call callback(name_ptr, first_cluster, file_size)
    push ecx
    push esi
    push edi

    push dword [edi + DIR_FileSize]
    movzx eax, word [edi + DIR_FstClusLO]
    push eax
    push edi
    call esi
    add  esp, 12

    pop  edi
    pop  esi
    pop  ecx

.next:
    add  edi, 32
    dec  ecx
    jmp  .loop

.done:
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; fat16_cd
;
;   push dword <name_ptr>   ; pointer to 11-byte 8.3 directory name
;                           ; pass 0 to go to root
;   call fat16_cd
;   add  esp, 4
;   ; EAX = 1 success, 0 = not found / not a directory
;
; Updates _cwd_cluster on success.
; ----------------------------------------------------------------------------
fat16_cd:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]           ; name pointer (or 0 for root)

    ; cd to root?
    test esi, esi
    jz   .go_root

    ; search cwd for a directory entry matching the name
    mov  ebx, [_cwd_cluster]    ; 0 = root

    ; we reuse the same sector-walking logic but look for ATTR_DIRECTORY
    ; and a matching name

    cmp  ebx, 0
    je   .search_root

    ; search a subdirectory cluster chain
.search_clus_loop:
    cmp  ebx, 2
    jb   .not_found

    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    mov  edi, [_sec_per_clus]
.search_sec_loop:
    push edi
    push ebx
    push eax

    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ebx
    pop  edi

    push eax
    push ebx
    push edi

    mov  edi, _sector_buf
    call _find_dir_entry        ; esi=name, edi=sector buf; returns EAX=cluster or 0
    test eax, eax
    jnz  .found_subdir          ; subdir path has 3 items on stack (eax, ebx, edi)

    pop  edi
    pop  ebx
    pop  eax

    inc  eax
    dec  edi
    jnz  .search_sec_loop

    call _fat16_next_cluster
    mov  ebx, eax
    cmp  ebx, FAT16_EOC
    jae  .not_found
    cmp  ebx, 0
    je   .not_found
    jmp  .search_clus_loop

.search_root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  ebx, eax

    mov  eax, [_root_lba]

.search_root_loop:
    push ebx
    push eax

    push dword _sector_buf
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ebx

    push eax
    push ebx

    mov  edi, _sector_buf
    call _find_dir_entry
    test eax, eax
    jnz  .found_root            ; root path has 2 items on stack (eax, ebx)

    pop  ebx
    pop  eax

    inc  eax
    dec  ebx
    jnz  .search_root_loop
    jmp  .not_found

.found_subdir:
    add  esp, 12                ; clean eax + ebx + edi (3 pushes)
    jmp  .found_common

.found_root:
    add  esp, 8                 ; clean eax + ebx (2 pushes)

.found_common:
    mov  [_cwd_cluster], eax
    mov  edi, _cwd_path
    mov  ecx, [_cwd_path_len]
    add  edi, ecx
    cmp  byte [edi-1], '/'
    je   .no_slash
    mov  byte [edi], '/'
    inc  edi
    inc  dword [_cwd_path_len]
.no_slash:
    push esi
    mov  ecx, 8
.copy_cd_name:
    mov  al, [esi]
    cmp  al, ' '
    je   .cd_name_end
    cmp  al, 0
    je   .cd_name_end
    mov  [edi], al
    inc  edi
    inc  esi
    inc  dword [_cwd_path_len]
    loop .copy_cd_name
.cd_name_end:
    mov  byte [edi], 0
    pop  esi
    mov  eax, 1
    jmp  .done

.go_root:
    mov  dword [_cwd_cluster], 0
    mov  byte [_cwd_path],     '/'
    mov  byte [_cwd_path + 1], 0
    mov  dword [_cwd_path_len], 1
    mov  eax, 1
    jmp  .done

.not_found:
    xor  eax, eax

.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _find_dir_entry (internal)
;   esi = pointer to 11-byte name to search for
;   edi = pointer to start of a 512-byte sector buffer
;   Returns EAX = first cluster if found and entry is a directory, else 0
; ----------------------------------------------------------------------------
_find_dir_entry:
    push ebx
    push ecx
    push esi
    push edi

    mov  ecx, 16

.loop:
    cmp  byte [edi], 0x00
    je   .not_found
    cmp  byte [edi], 0xE5
    je   .next

    ; must be a directory
    movzx edx, byte [edi + DIR_Attr]
    test dl, ATTR_DIRECTORY
    jz   .next

    ; compare name
    push ecx
    push edi
    push esi
    mov  ecx, 11
    repe cmpsb
    pop  esi
    pop  edi
    pop  ecx
    jne  .next

    ; found
    movzx eax, word [edi + DIR_FstClusLO]
    jmp  .done

.next:
    add  edi, 32
    dec  ecx
    jnz  .loop

.not_found:
    xor  eax, eax

.done:
    pop  edi
    pop  esi
    pop  ecx
    pop  ebx
    ret

; ============================================================================
; fat16_mkdir
;
;   push dword <name_ptr>   ; pointer to 11-byte uppercase space-padded 8.3 name
;   call fat16_mkdir
;   add  esp, 4
;   ; EAX = 1 success, 0 = failed (disk full / name exists)
;
; Creates a subdirectory in the current working directory.
; Steps:
;   1. Allocate a free cluster for the new directory's data.
;   2. Write that cluster's sector(s) with . and .. entries, rest zeroed.
;   3. Mark the cluster as end-of-chain in the FAT.
;   4. Add a 32-byte directory entry in the parent directory.
; ============================================================================

global fat16_mkdir

extern ata_write_sector
extern ata_write_sectors

section .bss
_mkdir_sector:  resb 512    ; scratch sector for mkdir operations
_clus_buf:      resb 4096   ; scratch buffer for writing one cluster (max 8 sec)

section .text

fat16_mkdir:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    ; ── 1. Find a free cluster ────────────────────────────────────────────
    call _alloc_cluster         ; returns cluster in EAX, 0 = disk full
    test eax, eax
    jz   .fail
    mov  ebx, eax               ; ebx = new cluster number

    ; ── 2. Zero the cluster sector(s) and write . and .. entries ─────────
    ; Clear _mkdir_sector
    mov  edi, _mkdir_sector
    mov  ecx, 512 / 4
    xor  eax, eax
    rep  stosd

    ; Write '.' entry at offset 0
    mov  edi, _mkdir_sector
    ; name: ".          " (dot + 10 spaces)
    mov  byte [edi + 0],  '.'
    mov  ecx, 10
    mov  al, ' '
    mov  edi, _mkdir_sector + 1
    rep  stosb
    mov  byte [_mkdir_sector + DIR_Attr], ATTR_DIRECTORY
    mov  ax, bx                 ; first cluster = this cluster
    mov  word [_mkdir_sector + DIR_FstClusLO], ax
    ; size = 0 for directories

    ; Write '..' entry at offset 32
    mov  byte [_mkdir_sector + 32 + 0], '.'
    mov  byte [_mkdir_sector + 32 + 1], '.'
    mov  ecx, 9
    mov  al, ' '
    mov  edi, _mkdir_sector + 32 + 2
    rep  stosb
    mov  byte [_mkdir_sector + 32 + DIR_Attr], ATTR_DIRECTORY
    ; parent cluster: 0 if parent is root, else _cwd_cluster
    mov  eax, [_cwd_cluster]
    mov  word [_mkdir_sector + 32 + DIR_FstClusLO], ax

    ; Write the sector(s) of the new cluster
    mov  eax, ebx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]       ; LBA of first sector of new cluster

    ; write each sector of the new cluster
    ; edi = sector counter (sec_per_clus down to 0)
    ; eax = current LBA (increments each iteration)
    mov  edi, [_sec_per_clus]
.write_clus_loop:
    push eax                        ; save LBA across write call
    push edi                        ; save sector counter across write call
    push dword _mkdir_sector        ; arg2: src buf
    push eax                        ; arg1: lba
    call ata_write_sector
    add  esp, 8                     ; clean 2 args
    pop  edi                        ; restore sector counter
    pop  eax                        ; restore LBA

    ; zero the . and .. area so all sectors after the first are clean
    push eax
    push edi
    mov  edi, _mkdir_sector
    xor  eax, eax
    mov  ecx, 16                    ; 16 dwords = 64 bytes = 2 entries
    rep  stosd
    pop  edi
    pop  eax

    inc  eax                        ; next LBA
    dec  edi                        ; sectors remaining
    jnz  .write_clus_loop

    ; ── 3. Mark cluster as end-of-chain in FAT ────────────────────────────
    mov  eax, ebx
    mov  ecx, 0xFFFF            ; FAT16 EOC
    call _fat_write_entry       ; write FAT[ebx] = 0xFFFF

    ; ── 4. Add directory entry in parent ──────────────────────────────────
    mov  esi, [ebp+8]           ; 11-byte name pointer
    call _add_dir_entry         ; esi=name, ebx=cluster; returns EAX=1/0
    test eax, eax
    jz   .fail

    mov  eax, 1
    jmp  .done
.fail:
    xor  eax, eax
.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _alloc_cluster — scans FAT for a free entry (value 0x0000)
; Returns cluster number in EAX, or 0 if disk full
; ----------------------------------------------------------------------------
_alloc_cluster:
    push ebx
    push ecx
    push edx
    push edi

    ; iterate FAT sectors
    mov  ecx, [_fat_size]       ; number of FAT sectors
    mov  ebx, [_fat_lba]        ; first FAT sector LBA
    mov  edi, 2                 ; first usable cluster number

.fat_sector_loop:
    push ecx
    push ebx

    push dword _fat_cache
    push dword 1
    push ebx
    call ata_read_sectors
    add  esp, 12

    pop  ebx
    pop  ecx

    ; each sector holds bytes_per_sec/2 FAT16 entries
    mov  edx, [_bytes_per_sec]
    shr  edx, 1                 ; total entries per sector

    push esi
    mov  esi, _fat_cache

    ; on the very first FAT sector, skip entries 0 and 1 (reserved)
    ; cluster 0 = media descriptor, cluster 1 = reserved
    ; edi starts at 2, so on the first sector we must advance esi by 4 bytes
    ; to align FAT index with cluster number edi.
    ; We detect the first sector by checking if edi == 2.
    cmp  edi, 2
    jne  .scan                  ; not the first sector — esi already aligned
    add  esi, 4                 ; skip FAT[0] and FAT[1]
    sub  edx, 2                 ; 2 fewer entries to scan this sector

.scan:
.entry_loop:
    cmp  word [esi], 0x0000     ; free cluster?
    je   .found
    add  esi, 2
    inc  edi                    ; advance cluster number in lockstep
    dec  edx
    jnz  .entry_loop
    pop  esi

    inc  ebx
    dec  ecx
    jnz  .fat_sector_loop

    xor  eax, eax               ; disk full
    jmp  .alloc_done

.found:
    pop  esi
    mov  eax, edi               ; return correct cluster number

.alloc_done:
    pop  edi
    pop  edx
    pop  ecx
    pop  ebx
    ret

; ----------------------------------------------------------------------------
; _fat_write_entry — writes a value into both FAT copies
;   EBX = cluster number
;   ECX = value to write (e.g. 0xFFFF for EOC, 0x0000 for free)
; Preserves EBX, ECX, EDX.
; ----------------------------------------------------------------------------
_fat_write_entry:
    push eax
    push ecx
    push edx
    push edi

    ; compute FAT byte offset and sector index
    mov  eax, ebx
    shl  eax, 1                         ; byte offset = cluster * 2
    xor  edx, edx
    div  dword [_bytes_per_sec]         ; eax = sector index in FAT, edx = byte offset

    mov  [_fwe_sector_idx], eax         ; save relative sector index
    mov  [_fwe_byte_off],   edx         ; save byte offset within sector
    mov  [_fwe_value],      cx          ; save value to write (ata calls trash ecx)
    mov  edi, [_fat_size]               ; fat_size saved in register before calls

    ; ── FAT copy 1 ───────────────────────────────────────────────────────────
    add  eax, [_fat_lba]                ; absolute LBA of FAT1 sector
    mov  [_fwe_lba], eax               ; save LBA — ata_read_sectors clobbers eax

    push dword _fat_cache
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    ; reload everything — eax, ecx, edx all clobbered by ata call
    mov  eax, [_fwe_lba]
    mov  edx, [_fwe_byte_off]
    movzx ecx, word [_fwe_value]
    mov  word [_fat_cache + edx], cx    ; patch the entry

    push dword _fat_cache
    push eax                            ; FAT1 LBA (safe — reloaded above)
    call ata_write_sector
    add  esp, 8

    ; ── FAT copy 2 ───────────────────────────────────────────────────────────
    mov  eax, [_fwe_sector_idx]
    add  eax, [_fat_lba]
    add  eax, edi                       ; + fat_size = FAT2 start
    mov  [_fwe_lba], eax

    push dword _fat_cache
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    mov  eax, [_fwe_lba]
    mov  edx, [_fwe_byte_off]
    movzx ecx, word [_fwe_value]
    mov  word [_fat_cache + edx], cx

    push dword _fat_cache
    push eax
    call ata_write_sector
    add  esp, 8

    ; invalidate read cache so next _fat16_next_cluster re-reads from disk
    mov  dword [_fat_cached_sec], 0xFFFFFFFF

    pop  edi
    pop  edx
    pop  ecx
    pop  eax
    ret

; ----------------------------------------------------------------------------
; _add_dir_entry — adds a 32-byte entry to the current directory
;   ESI = pointer to 11-byte name
;   EBX = first cluster of new entry
; Returns EAX = 1 success, 0 fail (directory full)
; ----------------------------------------------------------------------------
_add_dir_entry:
    push ebx
    push ecx
    push edx
    push esi
    push edi

    mov  ecx, [_cwd_cluster]    ; 0 = root

    cmp  ecx, 0
    je   .search_root

    ; search subdir cluster chain for a free (0x00 or 0xE5) entry
.subdir_loop:
    cmp  ecx, 2
    jb   .fail

    mov  eax, ecx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    mov  edi, [_sec_per_clus]   ; sector counter
.subdir_sec:
    ; eax = current LBA, ecx = cluster, edi = sectors remaining
    push eax                    ; save LBA — ata_read_sectors clobbers EDX via port I/O
    push edi
    push ecx

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  ecx
    pop  edi
    ; [esp] = saved LBA

    call _find_free_slot        ; scans _mkdir_sector, returns slot offset in EAX or -1
    cmp  eax, -1
    je   .subdir_next_sec

    ; found a free slot
    pop  edx                    ; restore LBA into EDX for _write_entry_to_slot
    call _write_entry_to_slot   ; EAX=slot offset, EDX=LBA, ESI=name, EBX=cluster
    test eax, eax
    jz   .fail
    jmp  .success

.subdir_next_sec:
    pop  edx                    ; discard saved LBA
    inc  edx                    ; advance to next LBA
    mov  eax, edx
    dec  edi
    jnz  .subdir_sec

    ; next cluster in chain
    push ebx
    mov  ebx, ecx
    call _fat16_next_cluster
    mov  ecx, eax
    pop  ebx
    cmp  ecx, FAT16_EOC
    jae  .fail
    cmp  ecx, 0
    je   .fail
    jmp  .subdir_loop

.search_root:
    ; search fixed root directory
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  edi, eax               ; root sector count

    mov  eax, [_root_lba]

.root_sec_loop:
    ; eax = current LBA, edi = sectors remaining
    push eax                    ; save LBA — ata_read_sectors clobbers EDX via port I/O
    push edi

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  edi
    ; [esp] = saved LBA

    call _find_free_slot        ; returns slot offset in EAX or -1
    cmp  eax, -1
    je   .root_next

    ; found — restore LBA into EDX for _write_entry_to_slot
    pop  edx
    call _write_entry_to_slot
    test eax, eax
    jz   .fail
    jmp  .success

.root_next:
    pop  edx                    ; discard saved LBA
    inc  edx                    ; next LBA
    mov  eax, edx
    dec  edi
    jnz  .root_sec_loop

.fail:
    xor  eax, eax
    jmp  .done
.success:
    mov  eax, 1
.done:
    pop  edi
    pop  esi
    pop  edx
    pop  ecx
    pop  ebx
    ret

; ----------------------------------------------------------------------------
; _find_free_slot — scans _mkdir_sector for a free (0x00 or 0xE5) dir entry
; Returns byte offset of the slot in EAX, or -1 if sector is full
; ----------------------------------------------------------------------------
_find_free_slot:
    push edi
    mov  edi, _mkdir_sector
    mov  ecx, 16
.loop:
    mov  al, [edi]
    cmp  al, 0x00
    je   .found
    cmp  al, 0xE5
    je   .found
    add  edi, 32
    loop .loop
    mov  eax, -1
    pop  edi
    ret
.found:
    mov  eax, edi
    sub  eax, _mkdir_sector     ; byte offset
    pop  edi
    ret

; ----------------------------------------------------------------------------
; _write_entry_to_slot
;   EAX = byte offset of free slot in _mkdir_sector
;   EDX = LBA of that sector
;   ESI = 11-byte name pointer
;   EBX = first cluster for the new entry
; Returns EAX = 1
; ----------------------------------------------------------------------------
_write_entry_to_slot:
    push ecx
    push edi

    ; clear the 32-byte slot
    lea  edi, [_mkdir_sector + eax]
    push edi
    push ecx
    mov  ecx, 32 / 4
    xor  eax, eax
    rep  stosd
    pop  ecx
    pop  edi

    ; copy 11-byte name
    push esi
    mov  ecx, 11
    rep  movsb
    pop  esi

    ; set attributes to ATTR_DIRECTORY
    ; edi now points 11 bytes into the slot; back up to slot base
    ; Actually after rep movsb edi is at slot+11 = DIR_Attr position
    mov  byte [edi], ATTR_DIRECTORY

    ; first cluster at DIR_FstClusLO (offset 26 from slot base)
    ; edi is currently at slot+11, need slot+26 = edi+15
    mov  word [edi + 15], bx    ; DIR_FstClusLO

    ; write sector back
    push dword _mkdir_sector
    push edx
    call ata_write_sector
    add  esp, 8

    mov  eax, 1
    pop  edi
    pop  ecx
    ret

; ============================================================================
; fat16_write_file
;
;   push dword <size>       ; number of bytes to write
;   push dword <src_buf>    ; source buffer
;   push dword <name_ptr>   ; 11-byte 8.3 name (file will be created/replaced)
;   call fat16_write_file
;   add  esp, 12
;   EAX = 1 success, 0 fail
;
; If the file already exists it is deleted first, then recreated.
; ============================================================================

global fat16_write_file
global fat16_delete
global fat16_rename

; scratch buffer for write operations (reuse _mkdir_sector)
; (defined already in bss as _mkdir_sector resb 512)

section .text

fat16_write_file:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    ; [ebp+8]  = name_ptr
    ; [ebp+12] = src_buf
    ; [ebp+16] = size (bytes)


    ; ── delete existing file if present ──────────────────────────────────
    push dword [ebp+8]
    call fat16_delete
    add  esp, 4


    ; ── allocate first cluster ────────────────────────────────────────────
    call _alloc_cluster
    test eax, eax
    jz   .fail
    mov  ebx, eax               ; ebx = first cluster (never changes)

    mov  esi, [ebp+12]          ; src read pointer
    mov  edi, [ebp+16]          ; bytes remaining

    mov  eax, ebx               ; eax = current cluster

.clus_loop:
    ; LBA = data_lba + (cluster-2) * sec_per_clus
    push eax
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]       ; eax = first LBA of this cluster
    push eax                    ; save LBA

    ; how many bytes does this cluster hold?
    mov  ecx, [_sec_per_clus]
    shl  ecx, 9                 ; * 512

    ; zero the cluster buffer
    push esi
    push edi
    push ecx
    mov  edi, _clus_buf
    mov  ecx, [_sec_per_clus]
    shl  ecx, 7                 ; * 512/4 = dwords in cluster
    xor  eax, eax
    rep  stosd
    pop  ecx                    ; cluster byte size
    pop  edi                    ; bytes_remaining
    pop  esi                    ; src ptr

    ; bytes_to_copy = min(bytes_remaining, cluster_size)
    cmp  edi, ecx
    jl   .partial
    jmp  .do_copy
.partial:
    mov  ecx, edi
.do_copy:
    ; copy ecx bytes from esi into _clus_buf
    push esi
    push edi
    push ecx
    mov  edi, _clus_buf
    rep  movsb
    pop  ecx
    pop  edi
    pop  esi

    sub  edi, ecx               ; bytes_remaining -= copied
    add  esi, ecx               ; advance src ptr

    ; write whole cluster in one ATA command
    pop  eax                    ; LBA
    push dword _clus_buf
    push dword [_sec_per_clus]
    push eax
    call ata_write_sectors
    add  esp, 12


    pop  eax                    ; restore current cluster

    ; done writing this cluster — any bytes left?
    test edi, edi
    jz   .chain_done


    ; allocate next cluster and link it
    push eax                    ; save current cluster
    call _alloc_cluster
    test eax, eax
    jz   .fail_chain

    ; link: FAT[current] = new_cluster
    pop  ecx                    ; ecx = current cluster
    push eax                    ; save new cluster
    push ebx
    mov  ebx, ecx
    mov  ecx, eax
    call _fat_write_entry       ; FAT[ebx] = ecx
    pop  ebx
    pop  eax                    ; eax = new cluster
    jmp  .clus_loop

.fail_chain:
    add  esp, 4
    jmp  .fail

.chain_done:
    ; mark last cluster EOC
    push ebx
    mov  ebx, eax
    mov  ecx, 0xFFFF
    call _fat_write_entry
    pop  ebx


    ; add directory entry
    mov  esi, [ebp+8]           ; name_ptr
    mov  edi, [ebp+16]          ; original file size
    call _add_file_entry        ; esi=name, ebx=first_cluster, edi=size

    test eax, eax
    jz   .fail

    mov  eax, 1
    jmp  .done
.fail:
    xor  eax, eax
.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _add_file_entry
;   ESI = 11-byte name, EBX = first cluster, EDI = file size
; Returns EAX = 1 success, 0 fail
; Stores file size in _file_entry_size to avoid stack tracking complexity.
; ----------------------------------------------------------------------------
_add_file_entry:
    push ebp
    mov  ebp, esp
    push ebx
    push ecx
    push edx
    push esi
    push edi

    ; stash size in a known location
    mov  [_file_entry_size], edi

    mov  ecx, [_cwd_cluster]
    cmp  ecx, 0
    je   .root

.subdir_loop:
    cmp  ecx, 2
    jb   .fail

    mov  eax, ecx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    mov  edx, [_sec_per_clus]   ; sector counter for this cluster
.subdir_sec:
    push edx
    push ecx
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ecx
    pop  edx

    push edx
    push ecx
    push eax

    push eax
    call _find_free_slot
    add  esp, 4
    cmp  eax, -1
    je   .subdir_next

    ; eax = slot offset, stack has [eax, ecx, edx]
    pop  edx                    ; LBA (was eax on stack = LBA)
    add  esp, 8                 ; discard saved ecx, edx (sector counter, cluster)
    call _write_file_entry_slot
    jmp  .done_ok

.subdir_next:
    pop  eax
    pop  ecx
    pop  edx

    inc  eax
    dec  edx
    jnz  .subdir_sec

    push ebx
    mov  ebx, ecx
    call _fat16_next_cluster
    mov  ecx, eax
    pop  ebx
    cmp  ecx, FAT16_EOC
    jae  .fail
    cmp  ecx, 0
    je   .fail
    jmp  .subdir_loop

.root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  edx, eax               ; sector count

    mov  eax, [_root_lba]

.root_loop:
    push edx
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  edx

    push edx
    push eax

    push eax
    call _find_free_slot
    add  esp, 4
    cmp  eax, -1
    je   .root_next

    pop  edx                    ; LBA
    add  esp, 4                 ; discard sector counter
    call _write_file_entry_slot
    jmp  .done_ok

.root_next:
    pop  eax
    pop  edx
    inc  eax
    dec  edx
    jnz  .root_loop

.fail:
    xor  eax, eax
    jmp  .done

.done_ok:
    mov  eax, 1

.done:
    pop  edi
    pop  esi
    pop  edx
    pop  ecx
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _write_file_entry_slot
;   EAX = byte offset of free slot in _mkdir_sector
;   EDX = LBA of that sector
;   ESI = 11-byte name
;   EBX = first cluster
;   reads size from [_file_entry_size]
; Returns EAX = 1
; ----------------------------------------------------------------------------
_write_file_entry_slot:
    push ecx
    push esi
    push edi

    ; zero the 32-byte slot
    lea  edi, [_mkdir_sector + eax]
    push edi
    mov  ecx, 32/4
    xor  eax, eax
    rep  stosd
    pop  edi                    ; edi = slot base

    ; copy 11-byte name
    mov  ecx, 11
    rep  movsb                  ; esi -> edi, both advance

    ; edi is now at slot+11 = DIR_Attr
    mov  byte [edi], 0x20       ; ARCHIVE

    ; DIR_FstClusLO = slot+26 = edi+15
    mov  word [edi+15], bx

    ; DIR_FileSize = slot+28 = edi+17
    mov  eax, [_file_entry_size]
    mov  dword [edi+17], eax

    ; write sector back
    push dword _mkdir_sector
    push edx
    call ata_write_sector
    add  esp, 8

    mov  eax, 1
    pop  edi
    pop  esi
    pop  ecx
    ret

; ============================================================================
; fat16_delete
;
;   push dword <name_ptr>   ; 11-byte 8.3 name
;   call fat16_delete
;   add  esp, 4
;   EAX = 1 deleted, 0 not found
;
; Marks the directory entry as 0xE5 and frees all clusters in the chain.
; ============================================================================
fat16_delete:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]           ; name_ptr
    mov  ecx, [_cwd_cluster]

    cmp  ecx, 0
    je   .root

.subdir_loop:
    cmp  ecx, 2
    jb   .not_found

    mov  eax, ecx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    mov  edi, [_sec_per_clus]
.subdir_sec:
    push edi
    push ecx
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ecx
    pop  edi

    push eax
    push ecx
    push edi

    push eax
    call _find_and_delete_entry  ; esi=name; returns EAX=first_cluster or 0
    add  esp, 4
    test eax, eax
    jnz  .free_chain

    pop  edi
    pop  ecx
    pop  eax

    inc  eax
    dec  edi
    jnz  .subdir_sec

    push ebx
    mov  ebx, ecx
    call _fat16_next_cluster
    mov  ecx, eax
    pop  ebx
    cmp  ecx, FAT16_EOC
    jae  .not_found
    cmp  ecx, 0
    je   .not_found
    jmp  .subdir_loop

.root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  edi, eax

    mov  eax, [_root_lba]

.root_loop:
    push edi
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  edi

    push eax
    push edi
    push eax
    call _find_and_delete_entry
    add  esp, 4
    test eax, eax
    jnz  .free_chain_root

    pop  edi
    pop  eax
    inc  eax
    dec  edi
    jnz  .root_loop

.not_found:
    xor  eax, eax
    jmp  .done

.free_chain_root:
    add  esp, 8

.free_chain:
    ; eax = first cluster of deleted entry; free the whole chain
    mov  ebx, eax
.free_loop:
    cmp  ebx, 2
    jb   .deleted
    cmp  ebx, FAT16_EOC
    jae  .deleted

    push ebx
    call _fat16_next_cluster    ; get next before we free
    push eax                    ; save next

    mov  ecx, 0x0000            ; free
    call _fat_write_entry       ; free ebx

    pop  eax
    pop  ebx
    mov  ebx, eax
    jmp  .free_loop

.deleted:
    mov  eax, 1

.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _find_and_delete_entry
;   [esp+4] = LBA of the sector (passed as arg)
;   ESI     = 11-byte name
;   _mkdir_sector already loaded
;   Marks matching entry as 0xE5 and writes sector back
;   Returns EAX = first cluster of deleted entry, or 0 if not found
; ----------------------------------------------------------------------------
_find_and_delete_entry:
    push ebp
    mov  ebp, esp
    push ecx
    push edi

    mov  edx, [ebp+8]           ; LBA from stack argument

    mov  edi, _mkdir_sector
    mov  ecx, 16

.loop:
    cmp  byte [edi], 0x00
    je   .not_found
    cmp  byte [edi], 0xE5
    je   .next

    movzx eax, byte [edi + DIR_Attr]
    cmp  al, ATTR_LFN
    je   .next
    test al, ATTR_VOLUME_ID
    jnz  .next

    push ecx
    push edi
    push esi
    mov  ecx, 11
    repe cmpsb
    pop  esi
    pop  edi
    pop  ecx
    jne  .next

    ; found — mark deleted
    movzx eax, word [edi + DIR_FstClusLO]
    mov  byte [edi], 0xE5

    push eax
    push dword _mkdir_sector    ; src buf
    push edx                    ; lba
    call ata_write_sector
    add  esp, 8
    pop  eax
    jmp  .done

.next:
    add  edi, 32
    loop .loop

.not_found:
    xor  eax, eax

.done:
    pop  edi
    pop  ecx
    pop  ebp
    ret

; ============================================================================
; fat16_rename
;
;   push dword <new_name_ptr>   ; 11-byte 8.3 new name
;   push dword <old_name_ptr>   ; 11-byte 8.3 old name
;   call fat16_rename
;   add  esp, 8
;   EAX = 1 success, 0 not found
;
; Simply updates the name field in the directory entry in-place.
; ============================================================================
fat16_rename:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]           ; old name
    mov  ecx, [_cwd_cluster]

    cmp  ecx, 0
    je   .root

.subdir_loop:
    cmp  ecx, 2
    jb   .not_found

    mov  eax, ecx
    sub  eax, 2
    imul eax, [_sec_per_clus]
    add  eax, [_data_lba]

    mov  edi, [_sec_per_clus]
.subdir_sec:
    push edi
    push ecx
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  ecx
    pop  edi

    push eax
    push ecx
    push edi

    push eax
    call _find_and_rename_entry
    test eax, eax
    jnz  .found

    pop  edx
    pop  edi
    pop  ecx
    pop  eax
    inc  eax
    dec  edi
    jnz  .subdir_sec

    push ebx
    mov  ebx, ecx
    call _fat16_next_cluster
    mov  ecx, eax
    pop  ebx
    cmp  ecx, FAT16_EOC
    jae  .not_found
    cmp  ecx, 0
    je   .not_found
    jmp  .subdir_loop

.root:
    mov  eax, [_root_entries]
    shl  eax, 5
    xor  edx, edx
    div  dword [_bytes_per_sec]
    mov  edi, eax

    mov  eax, [_root_lba]

.root_loop:
    push edi
    push eax

    push dword _mkdir_sector
    push dword 1
    push eax
    call ata_read_sectors
    add  esp, 12

    pop  eax
    pop  edi

    push eax
    push edi
    push eax
    call _find_and_rename_entry
    test eax, eax
    jnz  .found_root

    pop  edx
    pop  edi
    pop  eax
    inc  eax
    dec  edi
    jnz  .root_loop

.not_found:
    xor  eax, eax
    jmp  .done

.found_root:
    add  esp, 12
    jmp  .success
.found:
    add  esp, 16
.success:
    mov  eax, 1
.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret

; ----------------------------------------------------------------------------
; _find_and_rename_entry
;   ESI = old 11-byte name, [ebp+8] new name ptr (from fat16_rename stack),
;   but we pass new name via a global scratch to avoid complex stack threading.
;   Actually we pass new name via _rename_new_name which fat16_rename sets.
;   EDX = sector LBA
;   Returns EAX = 1 if renamed, 0 if not found
; ----------------------------------------------------------------------------
_find_and_rename_entry:
    push ebp
    mov  ebp, esp
    push ecx
    push edi
    mov  edx, [ebp+8]
    mov  edi, _mkdir_sector
    mov  ecx, 16

.loop:
    cmp  byte [edi], 0x00
    je   .not_found
    cmp  byte [edi], 0xE5
    je   .next

    movzx eax, byte [edi + DIR_Attr]
    cmp  al, ATTR_LFN
    je   .next
    test al, ATTR_VOLUME_ID
    jnz  .next

    push ecx
    push edi
    push esi
    mov  ecx, 11
    repe cmpsb
    pop  esi
    pop  edi
    pop  ecx
    jne  .next

    ; found — overwrite name with new name
    push esi
    push edi
    push ecx
    mov  esi, [_rename_new_ptr]
    mov  ecx, 11
    rep  movsb
    pop  ecx
    pop  edi
    pop  esi

    push dword _mkdir_sector
    push edx
    call ata_write_sector
    add  esp, 8

    mov  eax, 1
    jmp  .done

.next:
    add  edi, 32
    loop .loop

.not_found:
    xor  eax, eax
.done:
    pop  edi
    pop  ecx
    ret

section .bss
_rename_new_ptr:  resd 1
_file_entry_size: resd 1
_fwe_sector_idx:  resd 1
_fwe_byte_off:    resd 1
_fwe_value:       resw 1
_fwe_lba:         resd 1

section .text

; patch fat16_rename to set _rename_new_ptr before searching
; (we insert this at the top of fat16_rename by having it set the ptr)
; Already handled: fat16_rename sets [_rename_new_ptr] = new_name_ptr
; We need to add that line — patch fat16_rename entry point with a fixup:
; Actually we'll handle it by making fat16_rename store new_name_ptr first.
; The current fat16_rename doesn't do this — append a fixup trampoline:

; Redefine fat16_rename to store new ptr first, then call the real search.
; Since we can't redefine, we use a wrapper approach.
; The rename functions above already work — we just need fat16_rename
; to store [ebp+12] into _rename_new_ptr at its entry.
; Since NASM doesn't allow redefining, we'll patch via the existing code.
; The simplest fix: fat16_rename already has [ebp+12]=new_name at entry,
; just add one line. We do that by making the *first* thing in fat16_rename
; store it. Since we appended to the file after fat16_rename was defined,
; the cleanest approach is a trampoline — rename the old one and wrap it.
; For now, write a note: fat16_rename MUST store new name ptr.
; This is handled in the commands.asm wrapper _cmd_mv which sets
; _rename_new_ptr directly before calling fat16_rename.

global fat16_set_rename_ptr
fat16_set_rename_ptr:
    push ebp
    mov  ebp, esp
    mov  eax, [ebp+8]
    mov  [_rename_new_ptr], eax
    pop  ebp
    ret

; ============================================================================
; fat16_touch
;   push dword <name_ptr>   ; 11-byte 8.3 name
;   call fat16_touch
;   add  esp, 4
;   EAX = 1 success, 0 fail (name exists or directory full)
;
; Creates a zero-length file entry with no allocated clusters.
; If the file already exists this is a no-op (returns 1).
; ============================================================================
global fat16_touch

fat16_touch:
    push ebp
    mov  ebp, esp
    push ebx
    push esi
    push edi

    mov  esi, [ebp+8]

    ; if it already exists, do nothing
    push esi
    call fat16_open
    add  esp, 4
    test eax, eax
    jnz  .exists

    ; create entry: cluster=0, size=0
    mov  esi, [ebp+8]
    xor  ebx, ebx               ; no cluster
    xor  edi, edi               ; zero size
    call _add_file_entry        ; returns EAX=1/0
    jmp  .done

.exists:
    mov  eax, 1

.done:
    pop  edi
    pop  esi
    pop  ebx
    pop  ebp
    ret