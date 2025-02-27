/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */
#pragma once

#include <hardware/dma.h>

/*==============================================================================
 *  DMA Buffer Sizes
 *==============================================================================*/

#define DMA_RX_BUFFER_SIZE 1024
#define DMA_TX_BUFFER_SIZE 32

/*==============================================================================
 *  DMA Buffers
 *==============================================================================*/

extern uint8_t uart_rxbuf[DMA_RX_BUFFER_SIZE] __attribute__((aligned(DMA_RX_BUFFER_SIZE)));
extern uint8_t uart_txbuf[DMA_TX_BUFFER_SIZE] __attribute__((aligned(DMA_TX_BUFFER_SIZE)));

/*==============================================================================
 *  Ring Buffer Macro
 *==============================================================================*/

#define NEXT_RING_IDX(x) ((x + 1) & 0x3FF)
