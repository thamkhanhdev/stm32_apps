# FatFS
- If using DMA Template: need enabling NVIC SDMMC1 global interrupt for read/write via DMA
- If enable cache maintenance: need enabling the define "#define ENABLE_SD_DMA_CACHE_MAINTENANCE  1" in the file sd_diskio.c
# Integrate STM32CubeIDE