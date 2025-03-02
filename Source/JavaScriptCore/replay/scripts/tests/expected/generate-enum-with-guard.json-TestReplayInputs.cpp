/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// DO NOT EDIT THIS FILE. It is automatically generated from generate-enum-with-guard.json
// by the script: JavaScriptCore/replay/scripts/CodeGeneratorReplayInputs.py

#include "config.h"
#include "generate-enum-with-guard.json-TestReplayInputs.h"

#if ENABLE(WEB_REPLAY)
#include "InternalNamespaceImplIncludeDummy.h"
#include "PlatformWheelEvent.h"
#include <platform/ExternalNamespaceImplIncludeDummy.h>
#include <platform/PlatformWheelEvent.h>

namespace Test {
HandleWheelEvent::HandleWheelEvent(std::unique_ptr<PlatformWheelEvent> platformEvent, PlatformWheelPhase phase)
    : EventLoopInput<HandleWheelEvent>()
    , m_platformEvent(WTFMove(platformEvent))
    , m_phase(phase)
{
}

HandleWheelEvent::~HandleWheelEvent()
{
}
} // namespace Test

namespace JSC {
const String& InputTraits<Test::HandleWheelEvent>::type()
{
    static NeverDestroyed<const String> type(MAKE_STATIC_STRING_IMPL("HandleWheelEvent"));
    return type;
}

void InputTraits<Test::HandleWheelEvent>::encode(EncodedValue& encodedValue, const Test::HandleWheelEvent& input)
{
    encodedValue.put<WebCore::PlatformWheelEvent>(ASCIILiteral("platformEvent"), input.platformEvent());
    encodedValue.put<Test::PlatformWheelPhase>(ASCIILiteral("phase"), input.phase());
}

bool InputTraits<Test::HandleWheelEvent>::decode(EncodedValue& encodedValue, std::unique_ptr<Test::HandleWheelEvent>& input)
{
    std::unique_ptr<WebCore::PlatformWheelEvent> platformEvent;
    if (!encodedValue.get<WebCore::PlatformWheelEvent>(ASCIILiteral("platformEvent"), platformEvent))
        return false;

    Test::PlatformWheelPhase phase;
    if (!encodedValue.get<Test::PlatformWheelPhase>(ASCIILiteral("phase"), phase))
        return false;

    input = std::make_unique<Test::HandleWheelEvent>(WTFMove(platformEvent), phase);
    return true;
}
#if ENABLE(DUMMY_FEATURE)
EncodedValue EncodingTraits<Test::PlatformWheelPhase>::encodeValue(const Test::PlatformWheelPhase& enumValue)
{
    EncodedValue encodedValue = EncodedValue::createArray();
    if (enumValue & Test::PlatformWheelEventPhaseNone) {
        encodedValue.append<String>(ASCIILiteral("PlatformWheelEventPhaseNone"));
        if (enumValue == Test::PlatformWheelEventPhaseNone)
            return encodedValue;
    }
    return encodedValue;
}

bool EncodingTraits<Test::PlatformWheelPhase>::decodeValue(EncodedValue& encodedValue, Test::PlatformWheelPhase& enumValue)
{
    Vector<String> enumStrings;
    if (!EncodingTraits<Vector<String>>::decodeValue(encodedValue, enumStrings))
        return false;

    for (const String& enumString : enumStrings) {
        if (enumString == "PlatformWheelEventPhaseNone")
            enumValue = static_cast<Test::PlatformWheelPhase>(enumValue | Test::PlatformWheelEventPhaseNone);
    }

    return true;
}
#endif // ENABLE(DUMMY_FEATURE)
} // namespace JSC

#endif // ENABLE(WEB_REPLAY)
