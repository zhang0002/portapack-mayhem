/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
/*
    Creator: @htotoo
*/

#ifndef __PROC_WEATHER_H__
#define __PROC_WEATHER_H__

#include <algorithm>
#include "baseband_processor.hpp"
#include "baseband_thread.hpp"
#include "rssi_thread.hpp"
#include "message.hpp"
#include "dsp_decimate.hpp"

#include "fprotos/weatherprotos.hpp"

#define OOK_EST_HIGH_RATIO 3  // Constant for slowness of OOK high level estimator
#define OOK_EST_LOW_RATIO 5   // Constant for slowness of OOK low level (noise) estimator (very slow)
#define OOK_MAX_HIGH_LEVEL 450000

class WeatherProcessor : public BasebandProcessor {
   public:
    void execute(const buffer_c8_t& buffer) override;
    void on_message(const Message* const message) override;

   private:
    enum {
        STATE_IDLE = 0,
        STATE_PULSE = 1,
        STATE_GAP_START = 2,
        STATE_GAP = 3,
    } sig_state = STATE_IDLE;
    uint32_t low_estimate = 100;
    uint32_t high_estimate = 12000;
    uint32_t min_high_level = 10;
    size_t baseband_fs = 0;  // will be set later by configure message.
    uint32_t nsPerDecSamp = 0;
    uint8_t numg = 0;  // count of matched signals to filter spikes
    /* Array Buffer aux. used in decim0 and decim1 IQ c16 signed  data ; (decim0 defines the max length of the array) */
    std::array<complex16_t, 512> dst{};  // decim0 /4 ,  2048/4 = 512 complex I,Q
    const buffer_c16_t dst_buffer{
        dst.data(),
        dst.size()};

    /* Decimates */
    dsp::decimate::FIRC8xR16x24FS4Decim4 decim_0{};
    dsp::decimate::FIRC16xR16x16Decim2 decim_1{};

    uint32_t currentDuration = 0;
    uint32_t threshold = 0x0630;  // will overwrite after the first iteration
    bool currentHiLow = false;
    bool configured{false};

    FProtoListGeneral* protoList = new WeatherProtos();  // holds all the protocols we can parse
    void configure(const SubGhzFPRxConfigureMessage& message);
    void on_beep_message(const AudioBeepMessage& message);

    /* NB: Threads should be the last members in the class definition. */
    BasebandThread baseband_thread{baseband_fs, this, baseband::Direction::Receive};
    RSSIThread rssi_thread{};
};

#endif /*__PROC_WEATHER_H__*/
