/*
 * Copyright (C) 2016 Michal Debski <mi.zd.debski@gmail.com>
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"

#if ENABLE(WEB_DIAL)
#import "DiscoveryServiceChannelCocoa.h"

#include "Logging.h"
#include "NotImplemented.h"
#import <CoreFoundation/CoreFoundation.h>

using namespace WebCore;

@interface WebNSStreamDelegate : NSObject <NSStreamDelegate>
@end

@implementation WebNSStreamDelegate {
    DiscoveryServiceChannelCocoa *_provider;
}

- (id)initWithProvider:(DiscoveryServiceChannelCocoa*)provider
{
    if ( self = [super init]) {
        _provider = provider;
        return self;
    }
    return nil;
}

- (void)stream:(NSStream *)theStream handleEvent:(NSStreamEvent)streamEvent
{
    _provider->onStreamEvent(theStream, streamEvent);
}

@end

namespace WebCore {

Ref<DiscoveryServiceChannelCocoa> DiscoveryServiceChannelCocoa::create(RetainPtr<NSInputStream>&& inputStream, RetainPtr<NSOutputStream>&& outputStream)
{
    return adoptRef(*new DiscoveryServiceChannelCocoa(WTFMove(inputStream), WTFMove(outputStream)));
}

DiscoveryServiceChannelCocoa::DiscoveryServiceChannelCocoa(RetainPtr<NSInputStream>&& inputStream, RetainPtr<NSOutputStream>&& outputStream)
    : m_canSend(false)
    , m_inputStream(WTFMove(inputStream))
    , m_outputStream(WTFMove(outputStream))
{
    m_streamDelegate = adoptNS([[WebNSStreamDelegate alloc] initWithProvider:this]);
    [m_inputStream setDelegate: m_streamDelegate.get()];
    [m_outputStream setDelegate: m_streamDelegate.get()];

    [m_inputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [m_outputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

DiscoveryServiceChannelCocoa::~DiscoveryServiceChannelCocoa()
{
    [m_inputStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [m_outputStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

void DiscoveryServiceChannelCocoa::open()
{
    [m_inputStream open];
    [m_outputStream open];
}

void DiscoveryServiceChannelCocoa::reopen()
{
    notImplemented();
}

void DiscoveryServiceChannelCocoa::close()
{
    closeStreams();
    m_closedCallback();
}

void DiscoveryServiceChannelCocoa::closeStreams()
{
    [m_outputStream close];
    [m_inputStream close];
}

void DiscoveryServiceChannelCocoa::read()
{
    uint8_t* data;
    size_t size;
    Vector<uint8_t> buffer;
    if ([m_inputStream getBuffer:&data length:&size]) {
        LOG(WebDial, "Have a buffer of size %zu", size);
    } else {
        LOG(WebDial, "Need to provide our buffer");
        constexpr const auto defaultBufferSize = 32 * 1024u;
        while ([m_inputStream hasBytesAvailable]) {
            size = buffer.size();
            buffer.resize(size + defaultBufferSize);
            data = buffer.data() + size;
            NSInteger read = [m_inputStream read:data maxLength:defaultBufferSize];
            if (read == 0) {
                LOG(WebDial, "DiscoveryServiceChannelCocoa::read EOF");
                close();
                return;
            } else if (read < 0) {
                LOG(WebDial, "DiscoveryServiceChannelCocoa::read ERROR %s", [[[m_inputStream streamError] localizedDescription] UTF8String]);
                closeStreams();
                m_errorCallback([[m_inputStream streamError] localizedDescription]);
                return;
            }
            if (read < defaultBufferSize) {
                buffer.resize(size + read);
            }
        }
        data = buffer.data();
        size = buffer.size();
    }
    LOG(WebDial, "DiscoveryServiceChannelCocoa::read from %p %zu bytes", m_inputStream.get(), size);
    m_messageCallback(data, size);
}

size_t DiscoveryServiceChannelCocoa::send(const uint8_t* data, size_t size)
{
    if (!m_canSend) {
        LOG(WebDial, "DiscoveryServiceChannelCocoa::send cannot yet send - queue");
        Vector<uint8_t> buffer;
        buffer.append(data, size);
        m_queue.append(WTFMove(buffer));
        return size;
    }

    LOG(WebDial, "DiscoveryServiceChannelCocoa::send to %p", m_outputStream.get());
    m_canSend = false;
    NSInteger wrote = [m_outputStream write:data maxLength:size];
    if (wrote <= 0) {
        LOG(WebDial, "DiscoveryServiceChannelCocoa::send ERROR %s", [[[m_outputStream streamError] localizedDescription] UTF8String]);
        closeStreams();
        m_errorCallback([[m_outputStream streamError] localizedDescription]);
        return 0;
    }
    return wrote;
}

void DiscoveryServiceChannelCocoa::onStreamEvent(NSStream* stream, NSStreamEvent event)
{
    LOG(WebDial, "DiscoveryServiceChannelCocoa::onStreamEvent(%p, %x)", stream, event);
    switch (event) {
        case NSStreamEventNone:
        case NSStreamEventOpenCompleted:
            break;
        case NSStreamEventHasBytesAvailable:
            ASSERT(stream == m_inputStream.get());
            read();
            break;
        case NSStreamEventHasSpaceAvailable:
            ASSERT(stream == m_outputStream.get());
            m_canSend = true;
            if (!m_queue.isEmpty()) {
                Vector<uint8_t> buffer(m_queue.takeFirst());
                send(buffer.data(), buffer.size());
            }
            break;
        case NSStreamEventErrorOccurred:
            LOG(WebDial, "DiscoveryServiceChannelCocoa::onStreamEvent ERROR %s", [[[stream streamError] localizedDescription] UTF8String]);
            closeStreams();
            m_errorCallback([[stream streamError] localizedDescription]);
            break;
        case NSStreamEventEndEncountered:
            close();
            break;
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_DIAL)
