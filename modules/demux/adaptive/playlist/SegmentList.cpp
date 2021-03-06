/*
 * SegmentList.cpp
 *****************************************************************************
 * Copyright (C) 2010 - 2012 Klagenfurt University
 *
 * Created on: Jan 27, 2012
 * Authors: Christopher Mueller <christopher.mueller@itec.uni-klu.ac.at>
 *          Christian Timmerer  <christian.timmerer@itec.uni-klu.ac.at>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "SegmentList.h"
#include "Segment.h"
#include "SegmentInformation.hpp"

using namespace adaptive::playlist;

SegmentList::SegmentList( SegmentInformation *parent ):
    SegmentInfoCommon( parent ), TimescaleAble( parent )
{
}
SegmentList::~SegmentList()
{
    std::vector<ISegment *>::iterator it;
    for(it = segments.begin(); it != segments.end(); ++it)
        delete(*it);
}

const std::vector<ISegment*>& SegmentList::getSegments() const
{
    return segments;
}

ISegment * SegmentList::getSegmentByNumber(uint64_t number)
{
    std::vector<ISegment *>::const_iterator it = segments.begin();
    for(it = segments.begin(); it != segments.end(); ++it)
    {
        ISegment *seg = *it;
        if(seg->getSequenceNumber() == number)
        {
            return seg;
        }
        else if (seg->getSequenceNumber() > number)
        {
            break;
        }
    }
    return NULL;
}

void SegmentList::addSegment(ISegment *seg)
{
    seg->setParent(this);
    segments.push_back(seg);
}

void SegmentList::mergeWith(SegmentList *updated)
{
    const ISegment * lastSegment = (segments.empty()) ? NULL : segments.back();

    std::vector<ISegment *>::iterator it;
    for(it = updated->segments.begin(); it != updated->segments.end(); ++it)
    {
        if( !lastSegment || lastSegment->compare( *it ) < 0 )
            addSegment(*it);
        else
            delete *it;
    }
    updated->segments.clear();
}

void SegmentList::pruneByPlaybackTime(mtime_t time)
{
    uint64_t num;
    const uint64_t timescale = inheritTimescale();
    if(getSegmentNumberByScaledTime(time * timescale / CLOCK_FREQ, &num))
        pruneBySegmentNumber(num);
}

void SegmentList::pruneBySegmentNumber(uint64_t tobelownum)
{
    std::vector<ISegment *>::iterator it = segments.begin();
    while(it != segments.end())
    {
        ISegment *seg = *it;

        if(seg->getSequenceNumber() >= tobelownum)
            break;

        if(seg->chunksuse.Get()) /* can't prune from here, still in use */
            break;

        delete *it;
        it = segments.erase(it);
    }
}

bool SegmentList::getSegmentNumberByScaledTime(stime_t time, uint64_t *ret) const
{
    std::vector<ISegment *> allsubsegments;
    std::vector<ISegment *>::const_iterator it;
    for(it=segments.begin(); it!=segments.end(); ++it)
    {
        std::vector<ISegment *> list = (*it)->subSegments();
        allsubsegments.insert( allsubsegments.end(), list.begin(), list.end() );
    }

    return SegmentInfoCommon::getSegmentNumberByScaledTime(allsubsegments, time, ret);
}

mtime_t SegmentList::getPlaybackTimeBySegmentNumber(uint64_t number)
{
    if(segments.empty())
        return VLC_TS_INVALID;

    const uint64_t timescale = inheritTimescale();
    const ISegment *first = segments.front();
    if(first->getSequenceNumber() > number)
        return VLC_TS_INVALID;

    stime_t time = first->startTime.Get();
    std::vector<ISegment *>::iterator it = segments.begin();
    for(it = segments.begin(); it != segments.end(); ++it)
    {
        const ISegment *seg = *it;
        /* Assuming there won't be any discontinuity in sequence */
        if(seg->getSequenceNumber() == number)
        {
            break;
        }
        else if(seg->duration.Get())
        {
            time += seg->duration.Get();
        }
        else
        {
            time += duration.Get();
        }
    }

    return VLC_TS_0 + CLOCK_FREQ * time / timescale;
}
