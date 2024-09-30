	.section .section_disk,"a"
        .global _disk_start

_disk_start:
        .incbin __disk_file_path__
        .global _disk_end

_disk_end:    


