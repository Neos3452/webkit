/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "FileSystem.h"
#include <runtime/ArrayBuffer.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Variant.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if USE(CF)
#include <wtf/RetainPtr.h>
#endif

#if USE(SOUP)
#include "GUniquePtrSoup.h"
#endif

#if USE(FOUNDATION)
OBJC_CLASS NSArray;
OBJC_CLASS NSData;
#endif

namespace WebCore {

class SharedBufferDataView;

class WEBCORE_EXPORT SharedBuffer : public RefCounted<SharedBuffer> {
public:
    static Ref<SharedBuffer> create() { return adoptRef(*new SharedBuffer); }
    static Ref<SharedBuffer> create(const char* data, size_t size) { return adoptRef(*new SharedBuffer(data, size)); }
    static Ref<SharedBuffer> create(const unsigned char* data, size_t size) { return adoptRef(*new SharedBuffer(data, size)); }
    static RefPtr<SharedBuffer> createWithContentsOfFile(const String& filePath);

    static Ref<SharedBuffer> create(Vector<char>&&);
    
#if USE(FOUNDATION)
    RetainPtr<NSData> createNSData() const;
    RetainPtr<NSArray> createNSDataArray() const;
    static Ref<SharedBuffer> create(NSData *);
#endif
#if USE(CF)
    RetainPtr<CFDataRef> createCFData() const;
    static Ref<SharedBuffer> create(CFDataRef);
    void append(CFDataRef);
#endif

#if USE(SOUP)
    GUniquePtr<SoupBuffer> createSoupBuffer(unsigned offset = 0, unsigned size = 0);
    static Ref<SharedBuffer> wrapSoupBuffer(SoupBuffer*);
#endif

    // Calling data() causes all the data segments to be copied into one segment if they are not already.
    // Iterate the segments using begin() and end() instead.
    // FIXME: Audit the call sites of this function and replace them with iteration if possible.
    const char* data() const;

    // Creates an ArrayBuffer and copies this SharedBuffer's contents to that
    // ArrayBuffer without merging segmented buffers into a flat buffer.
    RefPtr<ArrayBuffer> tryCreateArrayBuffer() const;

    // FIXME: This should return a size_t.
    unsigned size() const { return m_size; }

    bool isEmpty() const { return !size(); }

    void append(const SharedBuffer&);
    void append(const char*, size_t);
    void append(Vector<char>&&);

    void clear();

    Ref<SharedBuffer> copy() const;

    // Data wrapped by a DataSegment should be immutable because it can be referenced by other objects.
    // To modify or combine the data, allocate a new DataSegment.
    class DataSegment : public ThreadSafeRefCounted<DataSegment> {
    public:
        const char* data() const;
        size_t size() const;

        static Ref<DataSegment> create(Vector<char>&& data) { return adoptRef(*new DataSegment(WTFMove(data))); }
#if USE(CF)
        static Ref<DataSegment> create(RetainPtr<CFDataRef>&& data) { return adoptRef(*new DataSegment(WTFMove(data))); }
#endif
#if USE(SOUP)
        static Ref<DataSegment> create(GUniquePtr<SoupBuffer>&& data) { return adoptRef(*new DataSegment(WTFMove(data))); }
#endif
        static Ref<DataSegment> create(MappedFileData&& data) { return adoptRef(*new DataSegment(WTFMove(data))); }

    private:
        DataSegment(Vector<char>&& data)
            : m_immutableData(WTFMove(data)) { }
#if USE(CF)
        DataSegment(RetainPtr<CFDataRef>&& data)
            : m_immutableData(WTFMove(data)) { }
#endif
#if USE(SOUP)
        DataSegment(GUniquePtr<SoupBuffer>&& data)
            : m_immutableData(WTFMove(data)) { }
#endif
        DataSegment(MappedFileData&& data)
            : m_immutableData(WTFMove(data)) { }

        Variant<Vector<char>,
#if USE(CF)
            RetainPtr<CFDataRef>,
#endif
#if USE(SOUP)
            GUniquePtr<SoupBuffer>,
#endif
            MappedFileData> m_immutableData;
        friend class SharedBuffer;
    };

    struct DataSegmentVectorEntry {
        size_t beginPosition;
        Ref<DataSegment> segment;
    };
    using DataSegmentVector = Vector<DataSegmentVectorEntry, 1>;
    DataSegmentVector::const_iterator begin() const { return m_segments.begin(); }
    DataSegmentVector::const_iterator end() const { return m_segments.end(); }
    
    // begin and end take O(1) time, this takes O(log(N)) time.
    SharedBufferDataView getSomeData(size_t position) const;

    void hintMemoryNotNeededSoon() const;

private:
    explicit SharedBuffer() = default;
    explicit SharedBuffer(const char*, size_t);
    explicit SharedBuffer(const unsigned char*, size_t);
    explicit SharedBuffer(Vector<char>&&);
    explicit SharedBuffer(MappedFileData&&);
#if USE(CF)
    explicit SharedBuffer(CFDataRef);
#endif
#if USE(SOUP)
    explicit SharedBuffer(SoupBuffer*);
#endif

    void combineIntoOneSegment() const;
    
    static RefPtr<SharedBuffer> createFromReadingFile(const String& filePath);

    size_t m_size { 0 };
    mutable DataSegmentVector m_segments;

#if !ASSERT_DISABLED
    mutable bool m_hasBeenCombinedIntoOneSegment { false };
    bool internallyConsistent() const;
#endif
};

class WEBCORE_EXPORT SharedBufferDataView {
public:
    SharedBufferDataView(Ref<SharedBuffer::DataSegment>&&, size_t);
    size_t size() const;
    const char* data() const;
private:
    size_t m_positionWithinSegment;
    Ref<SharedBuffer::DataSegment> m_segment;
};

RefPtr<SharedBuffer> utf8Buffer(const String&);

} // namespace WebCore
