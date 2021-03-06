// Copyright (c) 2010 The WebM project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include <strmif.h>
#include "mkvparserstreamaudio.hpp"
#include "mkvparser.hpp"
#include "vorbistypes.hpp"
#include "cmediatypes.hpp"
#include <cassert>
#include <limits>
#include <uuids.h>
#include <mmreg.h>
#include <malloc.h>
#ifdef _DEBUG
#include "odbgstream.hpp"
using std::endl;
#endif


namespace mkvparser
{

AudioStream* AudioStream::CreateInstance(const AudioTrack* pTrack)
{
    assert(pTrack);

    const char* const id = pTrack->GetCodecId();
    assert(id);  //TODO

    if (_stricmp(id, "A_VORBIS") == 0)
        __noop;
    else
        return 0;  //can't create a stream from this track

    //TODO: vet settings, etc

    AudioStream* const s = new (std::nothrow) AudioStream(pTrack);
    assert(s);  //TODO

    return s;
}


AudioStream::AudioStream(const AudioTrack* pTrack) :
    Stream(pTrack)
    //m_preroll(0)
{
}


std::wostream& AudioStream::GetKind(std::wostream& os) const
{
    return os << L"Audio";
}


BYTE AudioStream::GetChannels() const
{
    const AudioTrack* const pTrack = static_cast<const AudioTrack*>(m_pTrack);

    const __int64 channels = pTrack->GetChannels();
    assert(channels > 0);
    assert(channels <= 255);

    const BYTE result = static_cast<BYTE>(channels);
    return result;
}


ULONG AudioStream::GetSamplesPerSec() const
{
    const AudioTrack* const pTrack = static_cast<const AudioTrack*>(m_pTrack);

    const double rate = pTrack->GetSamplingRate();
    assert(rate > 0);

    double intrate_;
    const double fracrate = modf(rate, &intrate_);
    fracrate;
    assert(fracrate == 0);

    const ULONG result = static_cast<ULONG>(intrate_);
    return result;
}


BYTE AudioStream::GetBitsPerSample() const
{
    const AudioTrack* const pTrack = static_cast<const AudioTrack*>(m_pTrack);

    const __int64 val = pTrack->GetBitDepth();

    if (val <= 0)
        return 0;

    assert(val < 256);
    assert((val % 8) == 0);

    const BYTE result = static_cast<BYTE>(val);
    return result;
}


void AudioStream::GetMediaTypes(CMediaTypes& mtv) const
{
    mtv.Clear();

    const char* const id = m_pTrack->GetCodecId();
    assert(id);

    if (_stricmp(id, "A_VORBIS") == 0)
        GetVorbisMediaTypes(mtv);
    else
        assert(false);
}


void AudioStream::GetVorbisMediaTypes(CMediaTypes& mtv) const
{
    size_t cp_size;

    const BYTE* const cp = m_pTrack->GetCodecPrivate(cp_size);
    assert(cp);
    assert(cp_size > 0);

    const BYTE* const begin = &cp[0];
    const BYTE* const end = begin + cp_size;

    const BYTE* p = begin;
    assert(p < end);

    const BYTE n = *p++;
    n;
    assert(n == 2);
    assert(p < end);

    const ULONG id_len = *p++;  //TODO: don't assume < 255
    assert(id_len < 255);
    assert(id_len > 0);
    assert(p < end);

    const ULONG comment_len = *p++;  //TODO: don't assume < 255
    assert(comment_len < 255);
    assert(comment_len > 0);
    assert(p < end);

    //p points to first header

    const BYTE* const id_hdr = p;
    id_hdr;

    const BYTE* const comment_hdr = id_hdr + id_len;
    comment_hdr;

    const BYTE* const setup_hdr = comment_hdr + comment_len;
    setup_hdr;
    assert(setup_hdr < end);

    const ptrdiff_t setup_len_ = end - setup_hdr;
    assert(setup_len_ > 0);

    const DWORD setup_len = static_cast<DWORD>(setup_len_);

    const size_t hdr_len = id_len + comment_len + setup_len;

    using VorbisTypes::VORBISFORMAT2;

    const size_t cb = sizeof(VORBISFORMAT2) + hdr_len;
    BYTE* const pb = (BYTE*)_malloca(cb);

    VORBISFORMAT2& fmt = (VORBISFORMAT2&)(*pb);

    fmt.channels = GetChannels();
    fmt.samplesPerSec = GetSamplesPerSec();
    fmt.bitsPerSample = GetBitsPerSample();
    fmt.headerSize[0] = id_len;
    fmt.headerSize[1] = comment_len;
    fmt.headerSize[2] = setup_len;

    assert(p < end);
    assert(size_t(end - p) == hdr_len);

    BYTE* const dst = pb + sizeof(VORBISFORMAT2);
    memcpy(dst, p, hdr_len);

    AM_MEDIA_TYPE mt;

    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = VorbisTypes::MEDIASUBTYPE_Vorbis2;
    mt.bFixedSizeSamples = FALSE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = 0;
    mt.formattype = VorbisTypes::FORMAT_Vorbis2;
    mt.pUnk = 0;
    mt.cbFormat = static_cast<ULONG>(cb);
    mt.pbFormat = pb;

    mtv.Add(mt);

    //TODO: if we decide source filter should attempt to also
    //connect to Xiph Ogg Vorbis decoder filter:
    //mt.majortype = VorbisTypes::MEDIATYPE_OggPacketStream;
    //mt.subtype = MEDIASUBTYPE_None;
    //mt.bFixedSizeSamples = FALSE;
    //mt.bTemporalCompression = FALSE;
    //mt.lSampleSize = 0;
    //mt.formattype = VorbisTypes::FORMAT_OggIdentHeader;
    //mt.pUnk = 0;
    //mt.cbFormat = id_len;
    //mt.pbFormat = const_cast<BYTE*>(id_hdr);
    //
    //mtv.Add(mt);
}


HRESULT AudioStream::QueryAccept(const AM_MEDIA_TYPE* pmt) const
{
    if (pmt == 0)
        return E_INVALIDARG;

    const AM_MEDIA_TYPE& mt = *pmt;

    if (mt.majortype != MEDIATYPE_Audio)
        return S_FALSE;

    const char* const id = m_pTrack->GetCodecId();
    assert(id);

    if (_stricmp(id, "A_VORBIS") == 0)
    {
        if (mt.subtype != VorbisTypes::MEDIASUBTYPE_Vorbis2)
            return S_FALSE;

        return S_OK;
    }

    return S_FALSE;
}


#if 0  //if we decide to support Xiph Ogg Vorbis decoder filter:
HRESULT AudioStream::SetConnectionMediaType(const AM_MEDIA_TYPE&)
{
    if (mt.majortype == VorbisTypes::MEDIATYPE_OggPacketStream)
        m_preroll = &AudioStream::SendOggIdentPacket;
    else
        m_preroll = &AudioStream::DoNothing;

    return S_OK;
}
#endif


HRESULT AudioStream::UpdateAllocatorProperties(
    ALLOCATOR_PROPERTIES& props) const
{
    if (props.cBuffers <= 0)
        props.cBuffers = 1;

    const long size = GetBufferSize();

    if (props.cbBuffer < size)
        props.cbBuffer = size;

    if (props.cbAlign <= 0)
        props.cbAlign = 1;

    if (props.cbPrefix < 0)
        props.cbPrefix = 0;

    return S_OK;
}


long AudioStream::GetBufferSize() const
{
    const AudioTrack* const pTrack = static_cast<const AudioTrack*>(m_pTrack);

    const double rr = pTrack->GetSamplingRate();
    const long nSamplesPerSec = static_cast<long>(rr);

    const __int64 cc = pTrack->GetChannels();
    const long nChannels = static_cast<long>(cc);

    const long nBlockAlign = nChannels * 2;  //16-bit PCM
    const long nAvgBytesPerSec = nBlockAlign * nSamplesPerSec;

    const long size = nAvgBytesPerSec;  //TODO: make a better estimate

    return size;
}


void AudioStream::OnPopulateSample(
    const BlockEntry* pNextEntry,
    const samples_t& samples) const
{
    assert(!samples.empty());
    //assert(m_pBase);
    //assert(!m_pBase->EOS());
    assert(m_pCurr);
    assert(m_pCurr != m_pStop);
    assert(!m_pCurr->EOS());

    const Block* const pCurrBlock = m_pCurr->GetBlock();
    assert(pCurrBlock);
    assert(pCurrBlock->GetTrackNumber() == m_pTrack->GetNumber());

    const int nFrames = pCurrBlock->GetFrameCount();
    assert(nFrames > 0);  //checked by caller
    assert(samples.size() == samples_t::size_type(nFrames));

    const Cluster* const pCurrCluster = m_pCurr->GetCluster();
    assert(pCurrCluster);

    const __int64 start_ns = pCurrBlock->GetTime(pCurrCluster);
    assert(start_ns >= 0);
    //assert((start_ns % 100) == 0);

    const LONGLONG base_ns = m_base_time_ns;
    assert(base_ns >= 0);
    assert(start_ns >= base_ns);

    Segment* const pSegment = m_pTrack->m_pSegment;
    IMkvReader* const pFile = pSegment->m_pReader;

    __int64 stop_ns;

    if ((pNextEntry == 0) || pNextEntry->EOS())
    {
        const LONGLONG duration_ns = pSegment->GetDuration();

        if ((duration_ns >= 0) && (duration_ns > start_ns))
            stop_ns = duration_ns;
        else
        {
            const LONGLONG ns_per_frame = 10000000;  //10ms
            stop_ns = start_ns + LONGLONG(nFrames) * ns_per_frame;
        }
    }
    else
    {
        const Block* const pNextBlock = pNextEntry->GetBlock();
        assert(pNextBlock);

        const Cluster* const pNextCluster = pNextEntry->GetCluster();

        stop_ns = pNextBlock->GetTime(pNextCluster);
        assert(stop_ns > start_ns);
        //assert((stop_ns % 100) == 0);
    }

    __int64 start_reftime = (start_ns - base_ns) / 100;
    assert(start_ns >= 0);

    const __int64 block_stop_reftime = (stop_ns - base_ns) / 100;
    assert(block_stop_reftime > start_reftime);

    const __int64 block_duration = block_stop_reftime - start_reftime;
    assert(block_duration > 0);

    __int64 frame_duration = block_duration / nFrames;  //reftime units

    if (frame_duration <= 0)  //weird: block duration is very small
        frame_duration = 1;

    BOOL bDiscontinuity = m_bDiscontinuity ? TRUE : FALSE;

    for (int idx = 0; idx < nFrames; ++idx)
    {
        IMediaSample* const pSample = samples[idx];

        const Block::Frame& f = pCurrBlock->GetFrame(idx);

        const LONG srcsize = f.len;
        assert(srcsize >= 0);

        const long tgtsize = pSample->GetSize();
        tgtsize;
        assert(tgtsize >= 0);
        assert(tgtsize >= srcsize);

        BYTE* ptr;

        HRESULT hr = pSample->GetPointer(&ptr);
        assert(SUCCEEDED(hr));
        assert(ptr);

        const long status = f.Read(pFile, ptr);
        assert(status == 0);  //all bytes were read

        hr = pSample->SetActualDataLength(srcsize);

        hr = pSample->SetPreroll(FALSE);
        assert(SUCCEEDED(hr));

        hr = pSample->SetMediaType(0);
        assert(SUCCEEDED(hr));

        hr = pSample->SetDiscontinuity(bDiscontinuity);
        assert(SUCCEEDED(hr));

        bDiscontinuity = FALSE;

        hr = pSample->SetMediaTime(0, 0);
        assert(SUCCEEDED(hr));

        hr = pSample->SetSyncPoint(TRUE);
        assert(SUCCEEDED(hr));

        LONGLONG stop_reftime = start_reftime + frame_duration;

        hr = pSample->SetTime(&start_reftime, &stop_reftime);
        assert(SUCCEEDED(hr));

        start_reftime = stop_reftime;
    }
}


#if 0  //if we decide to support Xiph Ogg Vorbis decoder filter
bool AudioStream::SendPreroll(IMediaSample* pSample)
{
    assert(m_preroll);
    return (this->*m_preroll)(pSample);
}


bool AudioStream::DoNothing(IMediaSample*)
{
    return false;
}


bool AudioStream::SendOggIdentPacket(IMediaSample* pSample)
{
    assert(pSample);

    const char* const id = m_pTrack->GetCodecId();
    id;
    assert(id);
    assert(_stricmp(id, "A_VORBIS") == 0);

    const bytes_t& cp = m_pTrack->GetCodecPrivate();
    assert(!cp.empty());

    const ULONG cp_size = static_cast<ULONG>(cp.size());
    assert(cp_size > 0);

    const BYTE* const begin = &cp[0];
    const BYTE* const end = begin + cp_size;
    end;

    const BYTE* p = begin;
    assert(p < end);

    const BYTE n = *p++;
    n;
    assert(n == 2);
    assert(p < end);

    const ULONG id_len = *p++;  //TODO: don't assume < 255
    id_len;
    assert(id_len < 255);
    assert(id_len > 0);
    assert(p < end);

    const ULONG comment_len = *p++;  //TODO: don't assume < 255
    comment_len;
    assert(comment_len < 255);
    assert(comment_len > 0);
    assert(p < end);

    //p points to first header

    const BYTE* const id_hdr = p;

    const long size = pSample->GetSize();
    size;
    assert(size >= 0);
    assert(ULONG(size) >= id_len);

    BYTE* buf;

    HRESULT hr = pSample->GetPointer(&buf);
    assert(SUCCEEDED(hr));
    assert(buf);

    memcpy(buf, id_hdr, id_len);

    hr = pSample->SetActualDataLength(id_len);
    assert(SUCCEEDED(hr));

    hr = pSample->SetPreroll(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetMediaType(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetDiscontinuity(TRUE /* m_bDiscontinuity */ );  //TODO
    assert(SUCCEEDED(hr));

    //TODO
    //set by caller:
    //m_bDiscontinuity = false;

    hr = pSample->SetMediaTime(0, 0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetSyncPoint(FALSE);
    assert(SUCCEEDED(hr));

    hr = pSample->SetTime(0, 0);
    assert(SUCCEEDED(hr));

    m_preroll = &AudioStream::SendOggCommentPacket;

    return true;  //preroll - don't send payload
}


bool AudioStream::SendOggCommentPacket(IMediaSample* pSample)
{
    assert(pSample);

    const char* const id = m_pTrack->GetCodecId();
    id;
    assert(id);
    assert(_stricmp(id, "A_VORBIS") == 0);

    const bytes_t& cp = m_pTrack->GetCodecPrivate();
    assert(!cp.empty());

    const ULONG cp_size = static_cast<ULONG>(cp.size());
    assert(cp_size > 0);

    const BYTE* const begin = &cp[0];
    const BYTE* const end = begin + cp_size;
    end;

    const BYTE* p = begin;
    assert(p < end);

    const BYTE n = *p++;
    n;
    assert(n == 2);
    assert(p < end);

    const ULONG id_len = *p++;  //TODO: don't assume < 255
    assert(id_len < 255);
    assert(id_len > 0);
    assert(p < end);

    const ULONG comment_len = *p++;  //TODO: don't assume < 255
    assert(comment_len < 255);
    assert(comment_len > 0);
    assert(p < end);

    //p points to first header

    const BYTE* const id_hdr = p;
    id_hdr;

    const BYTE* const comment_hdr = id_hdr + id_len;
    comment_hdr;

    const long size = pSample->GetSize();
    size;
    assert(size >= 0);
    assert(ULONG(size) >= comment_len);

    BYTE* buf;

    HRESULT hr = pSample->GetPointer(&buf);
    assert(SUCCEEDED(hr));
    assert(buf);

    memcpy(buf, comment_hdr, comment_len);

    hr = pSample->SetActualDataLength(comment_len);
    assert(SUCCEEDED(hr));

    hr = pSample->SetPreroll(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetMediaType(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetDiscontinuity(TRUE /* m_bDiscontinuity */ );  //TODO
    assert(SUCCEEDED(hr));

    //TODO
    //set by caller:
    //m_bDiscontinuity = false;

    hr = pSample->SetMediaTime(0, 0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetSyncPoint(FALSE);
    assert(SUCCEEDED(hr));

    hr = pSample->SetTime(0, 0);
    assert(SUCCEEDED(hr));

    m_preroll = &AudioStream::SendOggSetupPacket;

    return true;  //preroll - don't send payload
}


bool AudioStream::SendOggSetupPacket(IMediaSample* pSample)
{
    assert(pSample);

    const char* const id = m_pTrack->GetCodecId();
    id;
    assert(id);
    assert(_stricmp(id, "A_VORBIS") == 0);

    const bytes_t& cp = m_pTrack->GetCodecPrivate();
    assert(!cp.empty());

    const ULONG cp_size = static_cast<ULONG>(cp.size());
    assert(cp_size > 0);

    const BYTE* const begin = &cp[0];
    const BYTE* const end = begin + cp_size;

    const BYTE* p = begin;
    assert(p < end);

    const BYTE n = *p++;
    n;
    assert(n == 2);
    assert(p < end);

    const ULONG id_len = *p++;  //TODO: don't assume < 255
    assert(id_len < 255);
    assert(id_len > 0);
    assert(p < end);

    const ULONG comment_len = *p++;  //TODO: don't assume < 255
    assert(comment_len < 255);
    assert(comment_len > 0);
    assert(p < end);

    //p points to first header

    const BYTE* const id_hdr = p;
    id_hdr;

    const BYTE* const comment_hdr = id_hdr + id_len;
    comment_hdr;

    const BYTE* const setup_hdr = comment_hdr + comment_len;
    setup_hdr;
    assert(setup_hdr < end);

    const ptrdiff_t setup_len_ = end - setup_hdr;
    assert(setup_len_ > 0);

    const ULONG setup_len = static_cast<ULONG>(setup_len_);

    const long size = pSample->GetSize();
    size;
    assert(size >= 0);
    assert(ULONG(size) >= setup_len);

    BYTE* buf;

    HRESULT hr = pSample->GetPointer(&buf);
    assert(SUCCEEDED(hr));
    assert(buf);

    memcpy(buf, setup_hdr, setup_len);

    hr = pSample->SetActualDataLength(setup_len);
    assert(SUCCEEDED(hr));

    hr = pSample->SetPreroll(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetMediaType(0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetDiscontinuity(TRUE /* m_bDiscontinuity */ );  //TODO
    assert(SUCCEEDED(hr));

    //TODO
    //set by caller:
    //m_bDiscontinuity = false;

    hr = pSample->SetMediaTime(0, 0);
    assert(SUCCEEDED(hr));

    hr = pSample->SetSyncPoint(FALSE);
    assert(SUCCEEDED(hr));

    hr = pSample->SetTime(0, 0);
    assert(SUCCEEDED(hr));

    m_preroll = &AudioStream::DoNothing;

    return true;  //don't send payload
}
#endif


}  //end namespace mkvparser
