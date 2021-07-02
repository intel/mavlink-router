/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2021 Patrick José Pereira <patrickelectric@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "tlog.h"

#include <assert.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>

#include <common/log.h>
#include <common/util.h>

bool TLog::_start_timeout()
{
    return true;
}

bool TLog::start()
{
    if (!LogEndpoint::start()) {
        return false;
    }

    return true;
}

void TLog::stop()
{
    if (_file == -1) {
        log_info("TLog not started");
        return;
    }

    LogEndpoint::stop();
}

int TLog::write_msg(const struct buffer *buffer)
{
    if (_file == -1 && !start()) {
        log_warning("Failed to initialize log.");
        return -1;
    }

    uint64_t ms_since_epoch =
        std::chrono::duration_cast<std::chrono::microseconds>
            (std::chrono::system_clock::now().time_since_epoch()).count();

    ms_since_epoch = htobe64(ms_since_epoch);

    write(_file, (void*)&ms_since_epoch, sizeof(uint64_t));
    write(_file, buffer->data, buffer->len);
    return buffer->len;
}
