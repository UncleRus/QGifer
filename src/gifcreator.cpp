/************************************************************************
 ** QGifer
 ** Copyright (C) 2013 Lukasz Chodyla <chodak166@op.pl>
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 ************************************************************************/

#include "gifcreator.h"
#include <cstring>

#include <iostream>
#include <QtDebug>
using namespace std;

// #define TRUE 1
// #define FALSE 0

void PrintGifError()
{
    qDebug() << "Something wrong whith GIF";
}

GifCreator::GifCreator() : duration(0.1), revStart(-1), revCmapStart(-1), canceled(false)
{
}

GifCreator::~GifCreator()
{
    // usuwanie powtórzeń
    for (int i = 0; i < cmaps.size() - 1; i++)
        for (int j = i + 1; j < cmaps.size(); j++)
            if (cmaps.at(i) == cmaps.at(j)) {
                cmaps.erase(cmaps.begin() + j);
                j--;
            }
    for (int i = 0; i < cmaps.size(); i++)
        GifFreeMapObject(cmaps[i]);
}

bool GifCreator::save(const char *filename, int every)
{
    canceled = false;
    if (!frames.size() || !cmaps.size()) {
        cerr << "ERROR: no frames or color map found!";
        cerr << "frames: " << frames.size() << "\nmaps: " << cmaps.size() << endl;
        return false;
    }

    if (every < 1)
        every = 1;

    int err;
    GifFileType *GifFile = EGifOpenFileName(filename, false, &err);

    if (!GifFile) {
        PrintGifError();
        return false;
    }

    auto fail = [&]() {
        PrintGifError();
        EGifCloseFile(GifFile, &err);
        endProgress();
        return false;
    };

    if (EGifPutScreenDesc(GifFile, w, h, colorRes, 0, cmaps.size() > 1 ? NULL : cmaps.at(0))
        == GIF_ERROR) {
        return fail();
    }

    // ------ loop ------
    int loop_count;
    loop_count = 0;
    {
        char nsle[12] = "NETSCAPE2.0";
        char subblock[3];
        // if (EGifPutExtensionFirst(GifFile, APPLICATION_EXT_FUNC_CODE, 11, nsle) == GIF_ERROR) {
        if (EGifPutExtensionLeader(GifFile, APPLICATION_EXT_FUNC_CODE) == GIF_ERROR) {
            return fail();
        }
        if (EGifPutExtensionBlock(GifFile, 11, nsle) == GIF_ERROR) {
            return fail();
        }
        subblock[0] = 1;
        subblock[2] = loop_count % 256;
        subblock[1] = loop_count / 256;
        if (EGifPutExtensionBlock(GifFile, 3, subblock) == GIF_ERROR) {
            return fail();
        }

        // if (EGifPutExtensionLast(GifFile, APPLICATION_EXT_FUNC_CODE, 3, subblock) == GIF_ERROR) {
        if (EGifPutExtensionTrailer(GifFile) == GIF_ERROR) {
            return fail();
        }
    }
    //------------------

    const int frameCount = static_cast<int>(frames.size());
    const int totalToSave = (frameCount + every - 1) / every;
    int savedCount = 0;

    for (int ni = 0; ni < frameCount; ni += every, ++savedCount) {
        const int progress = totalToSave > 0 ? (savedCount * 100) / totalToSave : 100;
        if (!savingProgress(progress)) {
            canceled = true;
            EGifCloseFile(GifFile, &err);
            endProgress();
            return false; // przerwanie przez usera
        }

        unsigned char ExtStr[4] = {0x04, 0x00, 0x00, 0xff};

        ExtStr[0] = (false) ? 0x06 : 0x04;
        // ExtStr[1] = delay[ni] % 256;
        // ExtStr[2] = delay[ni] / 256;

        int dt = duration * 100.0f;
        ExtStr[1] = dt % 256;
        ExtStr[2] = dt / 256;

        /* Dump graphics control block. */
        EGifPutExtension(GifFile, GRAPHICS_EXT_FUNC_CODE, 4, ExtStr);

        if (EGifPutImageDesc(GifFile, 0, 0, w, h, false,
                static_cast<int>(cmaps.size()) > ni ? cmaps.at(ni) : cmaps.at(cmaps.size() - 1))
            == GIF_ERROR) {
            return fail();
        }

        for (int y = 0, j = (h - 1) * w; y < h; y++, j -= w) {
            if (EGifPutLine(GifFile, &(frames[ni][j]), w) == GIF_ERROR) {
                return fail();
            }
        }
    }

    // comment
    if (EGifPutComment(GifFile, "This GIF file was created using QGifer") == GIF_ERROR) {
        return fail();
    }

    if (EGifCloseFile(GifFile, &err) == GIF_ERROR) {
        PrintGifError();
        endProgress();
        return false;
    }

    endProgress();
    return true;
}

void GifCreator::appendReversedCopy()
{
    if (frames.size() < 2) {
        return;
    }
    if (cmaps.size() > 1 && cmaps.size() != frames.size()) {
        cerr
            << "WARNING: going to append reversed copy of frames but palettes probably won't match!"
            << endl;
    }
    revStart = static_cast<int>(frames.size());
    revCmapStart = static_cast<int>(cmaps.size());
    for (int i = frames.size() - 2; i > 0; i--) {
        frames.push_back(frames.at(i));
    }

    if (cmaps.size() > 1)
        for (int i = cmaps.size() - 2; i > 0; i--) {
            cmaps.push_back(cmaps.at(i));
        }
}

void GifCreator::removeReversedCopy()
{
    if (revStart < 0)
        return;

    while (static_cast<int>(frames.size()) > revStart)
        frames.pop_back();
    if (revCmapStart >= 0) {
        while (static_cast<int>(cmaps.size()) > revCmapStart)
            cmaps.pop_back();
    }
    revStart = -1;
    revCmapStart = -1;
}
