/* 
 * tilelayer.cpp
 * Copyright 2008-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Jeff Bland <jksb@member.fsf.org>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tilelayer.h"

#include "map.h"
#include "tile.h"

using namespace Tiled;

TileLayer::TileLayer(const QString &name, int x, int y, int width, int height):
    Layer(TileLayerType, name, x, y, width, height),
    mMaxTileSize(0, 0),
    mGrid(width * height)
{
    Q_ASSERT(width >= 0);
    Q_ASSERT(height >= 0);
}

static QSize maxSize(const QSize &a,
                     const QSize &b)
{
    return QSize(qMax(a.width(), b.width()),
                 qMax(a.height(), b.height()));
}

static QMargins maxMargins(const QMargins &a,
                           const QMargins &b)
{
    return QMargins(qMax(a.left(), b.left()),
                    qMax(a.top(), b.top()),
                    qMax(a.right(), b.right()),
                    qMax(a.bottom(), b.bottom()));
}

/**
 * Recomputes the draw margins. Needed after the tile offset of a tileset
 * has changed for example.
 *
 * Generally you want to call Map::recomputeDrawMargins instead.
 */
void TileLayer::recomputeDrawMargins()
{
    QSize maxTileSize(0, 0);
    QMargins offsetMargins;

    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i) {
        const Cell &cell = mGrid.at(i);
        if (const Tile *tile = cell.tile) {
            QSize size = tile->size();

            if (cell.flippedAntiDiagonally)
                size.transpose();

            const QPoint offset = tile->offset();

            maxTileSize = maxSize(size, maxTileSize);
            offsetMargins = maxMargins(QMargins(-offset.x(),
                                                 -offset.y(),
                                                 offset.x(),
                                                 offset.y()),
                                        offsetMargins);
        }
    }

    mMaxTileSize = maxTileSize;
    mOffsetMargins = offsetMargins;

    if (mMap)
        mMap->adjustDrawMargins(drawMargins());
}

void TileLayer::setCell(int x, int y, const Cell &cell)
{
    Q_ASSERT(contains(x, y));

    if (cell.tile) {
        QSize size = cell.tile->size();

        if (cell.flippedAntiDiagonally)
            size.transpose();

        const QPoint offset = cell.tile->offset();

        mMaxTileSize = maxSize(size, mMaxTileSize);
        mOffsetMargins = maxMargins(QMargins(-offset.x(),
                                             -offset.y(),
                                             offset.x(),
                                             offset.y()),
                                    mOffsetMargins);

        if (mMap)
            mMap->adjustDrawMargins(drawMargins());
    }

    mGrid[x + y * mWidth] = cell;
}

TileLayer *TileLayer::copy(const QRegion &region) const
{
    const QRegion area = region.intersected(QRect(0, 0, width(), height()));
    const QRect bounds = region.boundingRect();
    const QRect areaBounds = area.boundingRect();
    const int offsetX = qMax(0, areaBounds.x() - bounds.x());
    const int offsetY = qMax(0, areaBounds.y() - bounds.y());

    TileLayer *copied = new TileLayer(QString(),
                                      0, 0,
                                      bounds.width(), bounds.height());

    foreach (const QRect &rect, area.rects())
        for (int x = rect.left(); x <= rect.right(); ++x)
            for (int y = rect.top(); y <= rect.bottom(); ++y)
                copied->setCell(x - areaBounds.x() + offsetX,
                                y - areaBounds.y() + offsetY,
                                cellAt(x, y));

    return copied;
}

void TileLayer::merge(const QPoint &pos, const TileLayer *layer)
{
    // Determine the overlapping area
    QRect area = QRect(pos, QSize(layer->width(), layer->height()));
    area &= QRect(0, 0, width(), height());

    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            const Cell &cell = layer->cellAt(x - pos.x(),
                                             y - pos.y());
            if (!cell.isEmpty())
                setCell(x, y, cell);
        }
    }
}

void TileLayer::setCells(int x, int y, TileLayer *layer,
                         const QRegion &mask)
{
    // Determine the overlapping area
    QRegion area = QRect(x, y, layer->width(), layer->height());
    area &= QRect(0, 0, width(), height());

    if (!mask.isEmpty())
        area &= mask;

    foreach (const QRect &rect, area.rects())
        for (int _x = rect.left(); _x <= rect.right(); ++_x)
            for (int _y = rect.top(); _y <= rect.bottom(); ++_y)
                setCell(_x, _y, layer->cellAt(_x - x, _y - y));
}

void TileLayer::erase(const QRegion &area)
{
    const Cell emptyCell;
    foreach (const QRect &rect, area.rects())
        for (int x = rect.left(); x <= rect.right(); ++x)
            for (int y = rect.top(); y <= rect.bottom(); ++y)
                setCell(x, y, emptyCell);
}

void TileLayer::flip(FlipDirection direction)
{
    QVector<Cell> newGrid(mWidth * mHeight);

    Q_ASSERT(direction == FlipHorizontally || direction == FlipVertically);

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            Cell &dest = newGrid[x + y * mWidth];
            if (direction == FlipHorizontally) {
                const Cell &source = cellAt(mWidth - x - 1, y);
                dest = source;
                dest.flippedHorizontally = !source.flippedHorizontally;
            } else if (direction == FlipVertically) {
                const Cell &source = cellAt(x, mHeight - y - 1);
                dest = source;
                dest.flippedVertically = !source.flippedVertically;
            }
        }
    }

    mGrid = newGrid;
}

void TileLayer::rotate(RotateDirection direction)
{
    static const char rotateRightMask[8] = { 5, 4, 1, 0, 7, 6, 3, 2 };
    static const char rotateLeftMask[8]  = { 3, 2, 7, 6, 1, 0, 5, 4 };

    const char (&rotateMask)[8] =
            (direction == RotateRight) ? rotateRightMask : rotateLeftMask;

    int newWidth = mHeight;
    int newHeight = mWidth;
    QVector<Cell> newGrid(newWidth * newHeight);

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            const Cell &source = cellAt(x, y);
            Cell dest = source;

            unsigned char mask =
                    (dest.flippedHorizontally << 2) |
                    (dest.flippedVertically << 1) |
                    (dest.flippedAntiDiagonally << 0);

            mask = rotateMask[mask];

            dest.flippedHorizontally = (mask & 4) != 0;
            dest.flippedVertically = (mask & 2) != 0;
            dest.flippedAntiDiagonally = (mask & 1) != 0;

            if (direction == RotateRight)
                newGrid[x * newWidth + (mHeight - y - 1)] = dest;
            else
                newGrid[(mWidth - x - 1) * newWidth + y] = dest;
        }
    }

    std::swap(mMaxTileSize.rwidth(),
              mMaxTileSize.rheight());

    mWidth = newWidth;
    mHeight = newHeight;
    mGrid = newGrid;
}


QSet<SharedTileset> TileLayer::usedTilesets() const
{
    QSet<SharedTileset> tilesets;

    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i)
        if (const Tile *tile = mGrid.at(i).tile)
            tilesets.insert(tile->sharedTileset());

    return tilesets;
}

bool TileLayer::referencesTileset(const Tileset *tileset) const
{
    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i) {
        const Tile *tile = mGrid.at(i).tile;
        if (tile && tile->tileset() == tileset)
            return true;
    }
    return false;
}

void TileLayer::removeReferencesToTileset(Tileset *tileset)
{
    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i) {
        const Tile *tile = mGrid.at(i).tile;
        if (tile && tile->tileset() == tileset)
            mGrid.replace(i, Cell());
    }
}

void TileLayer::replaceReferencesToTileset(Tileset *oldTileset,
                                           Tileset *newTileset)
{
    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i) {
        const Tile *tile = mGrid.at(i).tile;
        if (tile && tile->tileset() == oldTileset)
            mGrid[i].tile = newTileset->tileAt(tile->id());
    }
}

void TileLayer::resize(const QSize &size, const QPoint &offset)
{
    if (this->size() == size && offset.isNull())
        return;

    QVector<Cell> newGrid(size.width() * size.height());

    // Copy over the preserved part
    const int startX = qMax(0, -offset.x());
    const int startY = qMax(0, -offset.y());
    const int endX = qMin(mWidth, size.width() - offset.x());
    const int endY = qMin(mHeight, size.height() - offset.y());

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const int index = x + offset.x() + (y + offset.y()) * size.width();
            newGrid[index] = cellAt(x, y);
        }
    }

    mGrid = newGrid;
    setSize(size);
}

void TileLayer::offset(const QPoint &offset,
                       const QRect &bounds,
                       bool wrapX, bool wrapY)
{
    QVector<Cell> newGrid(mWidth * mHeight);

    for (int y = 0; y < mHeight; ++y) {
        for (int x = 0; x < mWidth; ++x) {
            // Skip out of bounds tiles
            if (!bounds.contains(x, y)) {
                newGrid[x + y * mWidth] = cellAt(x, y);
                continue;
            }

            // Get position to pull tile value from
            int oldX = x - offset.x();
            int oldY = y - offset.y();

            // Wrap x value that will be pulled from
            if (wrapX && bounds.width() > 0) {
                while (oldX < bounds.left())
                    oldX += bounds.width();
                while (oldX > bounds.right())
                    oldX -= bounds.width();
            }

            // Wrap y value that will be pulled from
            if (wrapY && bounds.height() > 0) {
                while (oldY < bounds.top())
                    oldY += bounds.height();
                while (oldY > bounds.bottom())
                    oldY -= bounds.height();
            }

            // Set the new tile
            if (contains(oldX, oldY) && bounds.contains(oldX, oldY))
                newGrid[x + y * mWidth] = cellAt(oldX, oldY);
            else
                newGrid[x + y * mWidth] = Cell();
        }
    }

    mGrid = newGrid;
}

bool TileLayer::canMergeWith(Layer *other) const
{
    return other->isTileLayer();
}

Layer *TileLayer::mergedWith(Layer *other) const
{
    Q_ASSERT(canMergeWith(other));

    const TileLayer *o = static_cast<TileLayer*>(other);
    const QRect unitedBounds = bounds().united(o->bounds());
    const QPoint offset = position() - unitedBounds.topLeft();

    TileLayer *merged = static_cast<TileLayer*>(clone());
    merged->resize(unitedBounds.size(), offset);
    merged->merge(o->position() - unitedBounds.topLeft(), o);
    return merged;
}

QRegion TileLayer::computeDiffRegion(const TileLayer *other) const
{
    QRegion ret;

    const int dx = other->x() - mX;
    const int dy = other->y() - mY;
    QRect r = QRect(0, 0, width(), height());
    r &= QRect(dx, dy, other->width(), other->height());

    for (int y = r.top(); y <= r.bottom(); ++y) {
        for (int x = r.left(); x <= r.right(); ++x) {
            if (cellAt(x, y) != other->cellAt(x - dx, y - dy)) {
                const int rangeStart = x;
                while (x <= r.right() &&
                       cellAt(x, y) != other->cellAt(x - dx, y - dy)) {
                    ++x;
                }
                const int rangeEnd = x;
                ret += QRect(rangeStart, y, rangeEnd - rangeStart, 1);
            }
        }
    }

    return ret;
}

bool TileLayer::isEmpty() const
{
    for (int i = 0, i_end = mGrid.size(); i < i_end; ++i)
        if (!mGrid.at(i).isEmpty())
            return false;

    return true;
}

/**
 * Returns a duplicate of this TileLayer.
 *
 * \sa Layer::clone()
 */
Layer *TileLayer::clone() const
{
    return initializeClone(new TileLayer(mName, mX, mY, mWidth, mHeight));
}

TileLayer *TileLayer::initializeClone(TileLayer *clone) const
{
    Layer::initializeClone(clone);
    clone->mGrid = mGrid;
    clone->mMaxTileSize = mMaxTileSize;
    clone->mOffsetMargins = mOffsetMargins;
    return clone;
}
