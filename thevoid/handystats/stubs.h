/*
 * Copyright (c) 2014 Yandex LLC. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IOREMAP_THEVOID_HANDYSTATS_STUBS_H
#define IOREMAP_THEVOID_HANDYSTATS_STUBS_H

#define HANDY_COUNTER_INIT(...)
#define HANDY_COUNTER_INCREMENT(...)
#define HANDY_COUNTER_DECREMENT(...)
#define HANDY_COUNTER_CHANGE(...)
#define HANDY_COUNTER_SCOPE(...)

#define HANDY_GAUGE_INIT(...)
#define HANDY_GAUGE_SET(...)

#define HANDY_TIMER_INIT(...)
#define HANDY_TIMER_START(...)
#define HANDY_TIMER_STOP(...)
#define HANDY_TIMER_DISCARD(...)
#define HANDY_TIMER_HEARTBEAT(...)
#define HANDY_TIMER_SCOPE(...)
#define HANDY_TIMER_SET(...)

#define HANDY_ATTRIBUTE_SET(...)
#define HANDY_ATTRIBUTE_SET_BOOL(...)
#define HANDY_ATTRIBUTE_SET_INT(...)
#define HANDY_ATTRIBUTE_SET_UINT(...)
#define HANDY_ATTRIBUTE_SET_INT64(...)
#define HANDY_ATTRIBUTE_SET_UINT64(...)
#define HANDY_ATTRIBUTE_SET_DOUBLE(...)
#define HANDY_ATTRIBUTE_SET_STRING(...)

#endif // IOREMAP_THEVOID_HANDYSTATS_STUBS_H
