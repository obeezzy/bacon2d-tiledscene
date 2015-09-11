#include "tiledlayer.h"
#include "tiledscene.h"
#include "tmxlayer.h"

TiledLayer::TiledLayer(QQuickItem *parent) :
    QQuickItem(parent)
  , m_layer(0)
{
}

TiledLayer::~TiledLayer()
{

}

void TiledLayer::setName(const QString &name)
{
    if(m_name == name)
        return;

    m_name = name;
    emit nameChanged();
}

QString TiledLayer::getProperty(const QString &name) const
{
    if(name == "opacity")
        return QString::number(m_layer->opacity());
    else if(name == "visible")
        return QString::number(m_layer->isVisible());

    return m_properties.value(name);
}

void TiledLayer::initialize()
{
    // Extract properties from layer
    TiledScene *scene = qobject_cast<TiledScene *>(parent());
    if(!scene || !scene->tiledMap())
        return;

    foreach(const TMXLayer &layer, scene->tiledMap()->layers())
    {
        if(layer.name() == m_name)
        {
            if(layer.isTileLayer())
            {
                TMXTileLayer tileLayer = static_cast<TMXTileLayer>(layer);
                setProperties(tileLayer.properties());

                setX(tileLayer.x());
                setY(tileLayer.y());
                setWidth(tileLayer.width());
                setHeight(tileLayer.height());

                setOpacity(tileLayer.opacity());
                setVisible(tileLayer.isVisible());

                setLayer(new TMXLayer(layer.layer(), this));
            }
            else if(layer.isImageLayer())
            {
                TMXImageLayer imageLayer = static_cast<TMXImageLayer>(layer);
                setProperties(imageLayer.properties());

                setX(imageLayer.x());
                setY(imageLayer.y());
                setWidth(imageLayer.width());
                setHeight(imageLayer.height());

                setOpacity(imageLayer.opacity());
                setVisible(imageLayer.isVisible());

                setLayer(new TMXLayer(layer.layer(), this));
            }
            else if(layer.isObjectLayer())
            {
                TMXObjectGroup objectGroup = static_cast<TMXObjectGroup>(layer);
                setProperties(objectGroup.properties());

                setX(objectGroup.x());
                setY(objectGroup.y());
                setWidth(objectGroup.width());
                setHeight(objectGroup.height());

                setOpacity(objectGroup.opacity());
                setVisible(objectGroup.isVisible());

                setLayer(new TMXLayer(layer.layer(), this));
            }
            else
                qWarning() << "Unknown layer type: " << layer.name();

            break;
        }
    }
}

void TiledLayer::setLayer(TMXLayer *layer)
{
    if(m_layer == layer)
        return;

    m_layer = layer;
    emit layerChanged();
}

QQmlListProperty<TiledObject> TiledLayer::objects()
{
    return QQmlListProperty<TiledObject>(this, 0,
                                        &TiledLayer::append_object,
                                        &TiledLayer::count_object,
                                        &TiledLayer::at_object,
                                        0);
}

void TiledLayer::append_object(QQmlListProperty<TiledObject> *list, TiledObject *object)
{
    TiledLayer *layer = static_cast<TiledLayer *>(list->object);
    object->setParent(layer);
    object->setParentItem(layer);
    connect(layer, SIGNAL(layerChanged()), object, SLOT(initialize()));
    layer->m_objects.append(object);
}

int TiledLayer::count_object(QQmlListProperty<TiledObject> *list)
{
    TiledLayer *layer = static_cast<TiledLayer *>(list->object);
    return layer->m_objects.length();
}

TiledObject *TiledLayer::at_object(QQmlListProperty<TiledObject> *list, int index)
{
    TiledLayer *layer = static_cast<TiledLayer *>(list->object);
    return layer->m_objects.at(index);
}
