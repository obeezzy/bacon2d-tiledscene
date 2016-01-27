/*
 * maptovariantconverter.cpp
 * Copyright 2011, Porfírio José Pereira Ribeiro <porfirioribeiro@gmail.com>
 * Copyright 2011-2015, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "maptovariantconverter.h"

#include "imagelayer.h"
#include "map.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "properties.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"
#include "terrain.h"

using namespace Tiled;

QVariant MapToVariantConverter::toVariant(const Map *map, const QDir &mapDir)
{
    mMapDir = mapDir;
    mGidMapper.clear();

    QVariantMap mapVariant;

    mapVariant[QLatin1String("version")] = 1.0;
    mapVariant[QLatin1String("orientation")] = orientationToString(map->orientation());
    mapVariant[QLatin1String("renderorder")] = renderOrderToString(map->renderOrder());
    mapVariant[QLatin1String("width")] = map->width();
    mapVariant[QLatin1String("height")] = map->height();
    mapVariant[QLatin1String("tilewidth")] = map->tileWidth();
    mapVariant[QLatin1String("tileheight")] = map->tileHeight();
    mapVariant[QLatin1String("properties")] = toVariant(map->properties());
    mapVariant[QLatin1String("nextobjectid")] = map->nextObjectId();

    if (map->orientation() == Map::Hexagonal) {
        mapVariant[QLatin1String("hexsidelength")] = map->hexSideLength();
    }

    if (map->orientation() == Map::Hexagonal || map->orientation() == Map::Staggered) {
        mapVariant[QLatin1String("staggeraxis")] = staggerAxisToString(map->staggerAxis());
        mapVariant[QLatin1String("staggerindex")] = staggerIndexToString(map->staggerIndex());
    }

    const QColor bgColor = map->backgroundColor();
    if (bgColor.isValid())
        mapVariant[QLatin1String("backgroundcolor")] = bgColor.name();

    QVariantList tilesetVariants;

    unsigned firstGid = 1;
    foreach (const SharedTileset &tileset, map->tilesets()) {
        tilesetVariants << toVariant(tileset.data(), firstGid);
        mGidMapper.insert(firstGid, tileset.data());
        firstGid += tileset->tileCount();
    }
    mapVariant[QLatin1String("tilesets")] = tilesetVariants;

    QVariantList layerVariants;
    foreach (const Layer *layer, map->layers()) {
        switch (layer->layerType()) {
        case Layer::TileLayerType:
            layerVariants << toVariant(static_cast<const TileLayer*>(layer),
                                       map->layerDataFormat());
            break;
        case Layer::ObjectGroupType:
            layerVariants << toVariant(static_cast<const ObjectGroup*>(layer));
            break;
        case Layer::ImageLayerType:
            layerVariants << toVariant(static_cast<const ImageLayer*>(layer));
            break;
        }
    }
    mapVariant[QLatin1String("layers")] = layerVariants;

    return mapVariant;
}

QVariant MapToVariantConverter::toVariant(const Tileset &tileset,
                                          const QDir &directory)
{
    mMapDir = directory;
    return toVariant(&tileset, 0);
}

QVariant MapToVariantConverter::toVariant(const Tileset *tileset,
                                          int firstGid) const
{
    QVariantMap tilesetVariant;

    if (firstGid > 0)
        tilesetVariant[QLatin1String("firstgid")] = firstGid;

    const QString &fileName = tileset->fileName();
    if (!fileName.isEmpty()) {
        QString source = mMapDir.relativeFilePath(fileName);
        tilesetVariant[QLatin1String("source")] = source;

        // Tileset is external, so no need to write any of the stuff below
        return tilesetVariant;
    }

    tilesetVariant[QLatin1String("name")] = tileset->name();
    tilesetVariant[QLatin1String("tilewidth")] = tileset->tileWidth();
    tilesetVariant[QLatin1String("tileheight")] = tileset->tileHeight();
    tilesetVariant[QLatin1String("spacing")] = tileset->tileSpacing();
    tilesetVariant[QLatin1String("margin")] = tileset->margin();
    tilesetVariant[QLatin1String("tilecount")] = tileset->tileCount();
    tilesetVariant[QLatin1String("properties")] = toVariant(tileset->properties());

    const QPoint offset = tileset->tileOffset();
    if (!offset.isNull()) {
        QVariantMap tileOffset;
        tileOffset[QLatin1String("x")] = offset.x();
        tileOffset[QLatin1String("y")] = offset.y();
        tilesetVariant[QLatin1String("tileoffset")] = tileOffset;
    }

    // Write the image element
    const QString &imageSource = tileset->imageSource();
    if (!imageSource.isEmpty()) {
        const QString rel = mMapDir.relativeFilePath(tileset->imageSource());

        tilesetVariant[QLatin1String("image")] = rel;

        const QColor transColor = tileset->transparentColor();
        if (transColor.isValid())
            tilesetVariant[QLatin1String("transparentcolor")] = transColor.name();

        tilesetVariant[QLatin1String("imagewidth")] = tileset->imageWidth();
        tilesetVariant[QLatin1String("imageheight")] = tileset->imageHeight();
    }

    // Write the properties, terrain, external image, object group and
    // animation for those tiles that have them.
    QVariantMap tilePropertiesVariant;
    QVariantMap tilesVariant;
    for (int i = 0; i < tileset->tileCount(); ++i) {
        const Tile *tile = tileset->tileAt(i);
        const Properties properties = tile->properties();
        if (!properties.isEmpty())
            tilePropertiesVariant[QString::number(i)] = toVariant(properties);
        QVariantMap tileVariant;
        if (tile->terrain() != 0xFFFFFFFF) {
            QVariantList terrainIds;
            for (int j = 0; j < 4; ++j)
                terrainIds << QVariant(tile->cornerTerrainId(j));
            tileVariant[QLatin1String("terrain")] = terrainIds;
        }
        if (tile->probability() != 1.f)
            tileVariant[QLatin1String("probability")] = tile->probability();
        if (!tile->imageSource().isEmpty()) {
            const QString rel = mMapDir.relativeFilePath(tile->imageSource());
            tileVariant[QLatin1String("image")] = rel;
        }
        if (tile->objectGroup())
            tileVariant[QLatin1String("objectgroup")] = toVariant(tile->objectGroup());
        if (tile->isAnimated()) {
            QVariantList frameVariants;
            foreach (const Frame &frame, tile->frames()) {
                QVariantMap frameVariant;
                frameVariant[QLatin1String("tileid")] = frame.tileId;
                frameVariant[QLatin1String("duration")] = frame.duration;
                frameVariants.append(frameVariant);
            }
            tileVariant[QLatin1String("animation")] = frameVariants;
        }

        if (!tileVariant.empty())
            tilesVariant[QString::number(i)] = tileVariant;
    }
    if (!tilePropertiesVariant.empty())
        tilesetVariant[QLatin1String("tileproperties")] = tilePropertiesVariant;
    if (!tilesVariant.empty())
        tilesetVariant[QLatin1String("tiles")] = tilesVariant;

    // Write terrains
    if (tileset->terrainCount() > 0) {
        QVariantList terrainsVariant;
        for (int i = 0; i < tileset->terrainCount(); ++i) {
            Terrain *terrain = tileset->terrain(i);
            const Properties &properties = terrain->properties();
            QVariantMap terrainVariant;
            terrainVariant[QLatin1String("name")] = terrain->name();
            if (!properties.isEmpty())
                terrainVariant[QLatin1String("properties")] = toVariant(properties);
            terrainVariant[QLatin1String("tile")] = terrain->imageTileId();
            terrainsVariant << terrainVariant;
        }
        tilesetVariant[QLatin1String("terrains")] = terrainsVariant;
    }

    return tilesetVariant;
}

QVariant MapToVariantConverter::toVariant(const Properties &properties) const
{
    QVariantMap variantMap;

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it)
        variantMap[it.key()] = it.value();

    return variantMap;
}

QVariant MapToVariantConverter::toVariant(const TileLayer *tileLayer,
                                          Map::LayerDataFormat format) const
{
    QVariantMap tileLayerVariant;
    tileLayerVariant[QLatin1String("type")] = QLatin1String("tilelayer");

    addLayerAttributes(tileLayerVariant, tileLayer);

    switch (format) {
    case Map::XML:
    case Map::CSV: {
        QVariantList tileVariants;
        for (int y = 0; y < tileLayer->height(); ++y)
            for (int x = 0; x < tileLayer->width(); ++x)
                tileVariants << mGidMapper.cellToGid(tileLayer->cellAt(x, y));

        tileLayerVariant[QLatin1String("data")] = tileVariants;
        break;
    }
    case Map::Base64:
    case Map::Base64Zlib:
    case Map::Base64Gzip: {
        tileLayerVariant[QLatin1String("encoding")] = QLatin1String("base64");

        if (format == Map::Base64Zlib)
            tileLayerVariant[QLatin1String("compression")] = QLatin1String("zlib");
        else if (format == Map::Base64Gzip)
            tileLayerVariant[QLatin1String("compression")] = QLatin1String("gzip");

        QByteArray layerData = mGidMapper.encodeLayerData(*tileLayer, format);
        tileLayerVariant[QLatin1String("data")] = layerData;
        break;
    }
    }

    return tileLayerVariant;
}

QVariant MapToVariantConverter::toVariant(const ObjectGroup *objectGroup) const
{
    QVariantMap objectGroupVariant;
    objectGroupVariant[QLatin1String("type")] = QLatin1String("objectgroup");

    if (objectGroup->color().isValid())
        objectGroupVariant[QLatin1String("color")] = objectGroup->color().name();

    objectGroupVariant[QLatin1String("draworder")] = drawOrderToString(objectGroup->drawOrder());

    addLayerAttributes(objectGroupVariant, objectGroup);
    QVariantList objectVariants;
    foreach (const MapObject *object, objectGroup->objects()) {
        QVariantMap objectVariant;
        const QString &name = object->name();
        const QString &type = object->type();

        objectVariant[QLatin1String("properties")] = toVariant(object->properties());
        objectVariant[QLatin1String("id")] = object->id();
        objectVariant[QLatin1String("name")] = name;
        objectVariant[QLatin1String("type")] = type;
        if (!object->cell().isEmpty())
            objectVariant[QLatin1String("gid")] = mGidMapper.cellToGid(object->cell());

        objectVariant[QLatin1String("x")] = object->x();
        objectVariant[QLatin1String("y")] = object->y();
        objectVariant[QLatin1String("width")] = object->width();
        objectVariant[QLatin1String("height")] = object->height();
        objectVariant[QLatin1String("rotation")] = object->rotation();

        objectVariant[QLatin1String("visible")] = object->isVisible();

        /* Polygons are stored in this format:
         *
         *   "polygon/polyline": [
         *       { "x": 0, "y": 0 },
         *       { "x": 1, "y": 1 },
         *       ...
         *   ]
         */
        const QPolygonF &polygon = object->polygon();
        if (!polygon.isEmpty()) {
            QVariantList pointVariants;
            foreach (const QPointF &point, polygon) {
                QVariantMap pointVariant;
                pointVariant[QLatin1String("x")] = point.x();
                pointVariant[QLatin1String("y")] = point.y();
                pointVariants.append(pointVariant);
            }

            if (object->shape() == MapObject::Polygon)
                objectVariant[QLatin1String("polygon")] = pointVariants;
            else
                objectVariant[QLatin1String("polyline")] = pointVariants;
        }

        if (object->shape() == MapObject::Ellipse)
            objectVariant[QLatin1String("ellipse")] = true;

        objectVariants << objectVariant;
    }

    objectGroupVariant[QLatin1String("objects")] = objectVariants;
    return objectGroupVariant;
}

QVariant MapToVariantConverter::toVariant(const ImageLayer *imageLayer) const
{
    QVariantMap imageLayerVariant;
    imageLayerVariant[QLatin1String("type")] = QLatin1String("imagelayer");

    addLayerAttributes(imageLayerVariant, imageLayer);

    const QString rel = mMapDir.relativeFilePath(imageLayer->imageSource());
    imageLayerVariant[QLatin1String("image")] = rel;

    const QColor transColor = imageLayer->transparentColor();
    if (transColor.isValid())
        imageLayerVariant[QLatin1String("transparentcolor")] = transColor.name();

    return imageLayerVariant;
}

void MapToVariantConverter::addLayerAttributes(QVariantMap &layerVariant,
                                               const Layer *layer) const
{
    layerVariant[QLatin1String("name")] = layer->name();
    layerVariant[QLatin1String("width")] = layer->width();
    layerVariant[QLatin1String("height")] = layer->height();
    layerVariant[QLatin1String("x")] = layer->x();
    layerVariant[QLatin1String("y")] = layer->y();
    layerVariant[QLatin1String("visible")] = layer->isVisible();
    layerVariant[QLatin1String("opacity")] = layer->opacity();

    const Properties &properties = layer->properties();
    if (!properties.isEmpty())
        layerVariant[QLatin1String("properties")] = toVariant(properties);
}
