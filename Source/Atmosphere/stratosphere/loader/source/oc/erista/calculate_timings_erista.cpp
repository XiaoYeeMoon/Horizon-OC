/*
 * Copyright (c) Lightos_
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../mtc_timing_value.hpp"

namespace ams::ldr::hoc::pcv::erista {

    void SwitchLatency(volatile u32 &latency, u32 index, u32 latencyStep) {
        latency += index * latencyStep;
    }

    static s32 GetMaxLatencyIndex(volatile u32 *latencyArray, u32 latencySize) {
        s32 maxIndex = -1;
        for (u32 i = 0; i < latencySize; ++i) {
            if (latencyArray[i]) {
                maxIndex = i;
            }
        }

        return maxIndex;
    }

    void AutoLatency(volatile u32 &latency, u32 freq, u32 latencyStep) {
        if (freq > 1600'000 && freq <= 1866'000) { /* 1866tRWL */
            latency += latencyStep * 2;
        } else { /* 2133tRWL */
            latency += latencyStep * 3;
        }
    }

    void HandleLatency(u32 freq, volatile u32 &latency, volatile u32 *latencyArray, u32 indexMax, u32 latencyStep) {
        for (u32 i = 0; i <= indexMax; ++i) {
            if (latencyArray[i] != 0 && freq <= latencyArray[i]) {
                SwitchLatency(latency, i, latencyStep);
                return;
            }
        }

        SwitchLatency(latency, indexMax, latencyStep);
    }

    void HandleLatency(u32 freq) {
        static s32 rlIndexMax = GetMaxLatencyIndex(C.readLatency, std::size(C.readLatency));
        static s32 wlIndexMax = GetMaxLatencyIndex(C.writeLatency, std::size(C.writeLatency));
        constexpr u32 ReadLatencyStep  = 4;
        constexpr u32 WriteLatencyStep = 2;
        bool autoLatencyRead = false, autoLatencyWrite = false;

        if (rlIndexMax == -1) {
            AutoLatency(RL, freq, ReadLatencyStep);
            autoLatencyRead = true;
        }

        if (wlIndexMax == -1) {
            AutoLatency(WL, freq, WriteLatencyStep);
            autoLatencyWrite = true;
        }

        if (autoLatencyRead && autoLatencyWrite) {
            return;
        }

        if (!autoLatencyRead) {
            HandleLatency(freq, RL, C.readLatency, rlIndexMax, ReadLatencyStep);
        }

        if (!autoLatencyWrite) {
            HandleLatency(freq, WL, C.writeLatency, wlIndexMax, WriteLatencyStep);
        }
    }

    void CalculateTimings(double tCK_avg, u32 freq) {
        RL = RL_1331;
        WL = WL_1331;

        HandleLatency(freq);

        tR2P = CEIL((RL * 0.426) - 2.0);
        tR2W = FLOOR(FLOOR((5.0 / tCK_avg) + ((FLOOR(48.0 / WL) - 0.478) * 3.0)) / 1.501) + RL - (C.t6_tRTW * 3) + finetRTW;

        tW2P    = (CEIL(WL * 1.7303) * 2) - 5;
        tWTPDEN = CEIL(((1.803 / tCK_avg) + MAX(RL + (2.694 / tCK_avg), static_cast<double>(tW2P))) + (BL / 2));
        tW2R    = FLOOR(MAX((5.020 / tCK_avg) + 1.130, WL - MAX(-CEIL(0.258 * (WL - RL)), 1.964)) * 1.964) + WL - CEIL(tWTR / tCK_avg) + finetWTR;

        pdex2rw  = CEIL((CEIL(12.335 - tCK_avg) + (7.430 / tCK_avg) - CEIL(tCK_avg * 11.361)));
        tCLKSTOP = FLOOR(MIN(8.488 / tCK_avg, 23.0)) + 8.0;

        const double tMMRI = tRCD + (tCK_avg * 3);
        pdex2mrr           = tMMRI + 10;
    }

}
