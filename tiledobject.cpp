#include "tiledobject.h"
#include "box2dbody.h"
#include "box2dfixture.h"
#include "tmxlayer.h"
#include "tiledlayer.h"
#include "tmxobjectgroup.h"
#include "tiledscene.h"
#include <QDebug>

TiledObject::TiledObject(QQuickItem *parent) :
    QQuickItem(parent)
  , m_objectGroup(0)
  , m_body(0)
  , m_collisionIndex(-1)
  , m_componentCompleted(false)
{
}

TiledObject::~TiledObject()
{
    qDeleteAll(m_fixtures);
    qDeleteAll(m_collisionItems);
}

void TiledObject::setName(const QString &name)
{
    if(m_name == name)
        return;

    m_name = name;
    emit nameChanged();
}

void TiledObject::setType(const QString &type)
{
    if(m_type == type)
        return;

    m_type = type;
    emit typeChanged();
}

void TiledObject::setBody(Box2DBody *body)
{
    if(m_body == body)
        return;

    m_body = body;
    emit bodyChanged();
}

QString TiledObject::getProperty(const QString &name) const
{
    if(m_objectGroup)
    {
        if(!m_type.isEmpty() && !m_name.isEmpty())
        {
            foreach(const TMXMapObject &object, m_objectGroup->objects())
            {
                if(object.name() != m_name || object.type() != m_type)
                    continue;

                if(name.toLower() == "x")
                    return QString::number(object.x());
                else if(name.toLower() == "y")
                    return QString::number(object.y());
                else if(name.toLower() == "width")
                    return QString::number(object.width());
                else if(name.toLower() == "height")
                    return QString::number(object.height());
                else if(name.toLower() == "rotation")
                    return QString::number(object.rotation());
                else if(name.toLower() == "visible")
                    return QString::number(object.isVisible());
            }
        }

        else if(!m_name.isEmpty())
        {
            foreach(const TMXMapObject &object, m_objectGroup->objects())
            {
                if(object.name() != m_name)
                    continue;

                if(name.toLower() == "x")
                    return QString::number(object.x());
                else if(name.toLower() == "y")
                    return QString::number(object.y());
                else if(name.toLower() == "width")
                    return QString::number(object.width());
                else if(name.toLower() == "height")
                    return QString::number(object.height());
                else if(name.toLower() == "rotation")
                    return QString::number(object.rotation());
                else if(name.toLower() == "visible")
                    return QString::number(object.isVisible());
            }
        }
    }

    return m_properties.value(name);
}

void TiledObject::initialize()
{
    if(!m_componentCompleted) {
        m_componentCompleted = true;
        return;
    }

    // Extract properties from layer
    TiledLayer *tiledLayer = qobject_cast<TiledLayer *>(parent());
    if(!tiledLayer && !tiledLayer->layer())
        return;
    if(!tiledLayer->layer()->isObjectLayer())
        return;

    m_layerName = tiledLayer->name();

    if(m_objectGroup)
        m_objectGroup->deleteLater();

    m_objectGroup = new TMXObjectGroup(*tiledLayer->layer(), this);
    if(!m_objectGroup)
        return;

    int collisions = 0;
    m_collisionIndex = -1;
    foreach(const TMXMapObject &object, m_objectGroup->objects())
    {
        if(object.name() == m_name && object.type() == m_type)
        {
            if(!static_cast<TiledScene *>(tiledLayer->parentItem()))
                return;

            //if(!collisions) {
                setProperties(object.properties());

                setX(object.x());
                setY(object.y());
                setWidth(object.width());
                setHeight(object.height());
                setRotation(object.rotation());

                setVisible(object.isVisible());
            //}

            CollisionItem *item = new CollisionItem(tiledLayer->parentItem());
            item->setX(object.x());
            item->setY(object.y());
            item->setWidth(object.width());
            item->setHeight(object.height());
            item->setRotation(object.rotation());
            item->setVisible(object.isVisible());
            item->setProperties(object.properties());
            item->setCollisionIndex(collisions);
            m_collisionItems.append(item);

            switch(object.shape())
            {
            case TMXMapObject::Rectangle:
                createRectangularFixture(object, item);
                break;
            case TMXMapObject::Ellipse:
                createEllipseFixture(object, item);
                break;
            case TMXMapObject::Polygon:
                createPolygonFixture(object, item);
                break;
            case TMXMapObject::Polyline:
                createPolylineFixture(object, item);
                break;
            default:
                qWarning() << "Unhandled object group: " << object.name();
                break;
            }

            setBody(item->body());
            collisions++;
        }
    }
}

void TiledObject::createRectangularFixture(const TMXMapObject &object, CollisionItem *item)
{
    if(!item)
        return;
    if(m_fixtures.isEmpty())
        return;

    Box2DBody *body = new Box2DBody(item);
    body->setBodyType(Box2DBody::Static);
    body->setTarget(item);
    body->setActive(true);
    body->setSleepingAllowed(false);

    foreach(Box2DFixture *objectFixture, m_fixtures)
    {
        if(!objectFixture)
            continue;

        // Check to see if the user of the library set the x, y, width and/or height values.
        Box2DBox *box = static_cast<Box2DBox *>(objectFixture);
        if(!box)
            return;

        Box2DBox *fixture = new Box2DBox(item);

        // Add x and y values set by user as offsets
        fixture->setX(box->x());
        fixture->setY(box->y());
        fixture->setWidth(box->width() == 0 ? object.width() : box->width());
        fixture->setHeight(box->height() == 0 ? object.height() : box->height());

        fixture->setDensity(box->density());
        fixture->setFriction(box->friction());
        fixture->setRestitution(box->restitution());
        fixture->setSensor(box->isSensor());
        fixture->setCategories(box->categories());
        fixture->setCollidesWith(box->collidesWith());
        fixture->setGroupIndex(box->groupIndex());

        connect(box, SIGNAL(beginContact(Box2DFixture*)), fixture, SIGNAL(beginContact(Box2DFixture*)));
        connect(box, SIGNAL(endContact(Box2DFixture*)), fixture, SIGNAL(endContact(Box2DFixture*)));

        body->addFixture(fixture);
    }

    body->componentComplete();
    item->setBody(body);
}

void TiledObject::createEllipseFixture(const TMXMapObject &object, CollisionItem *item)
{
    if(!item)
        return;
    if(m_fixtures.isEmpty())
        return;

    Box2DBody *body = new Box2DBody(item);
    body->setBodyType(Box2DBody::Static);
    body->setTarget(item);
    body->setActive(true);
    body->setSleepingAllowed(false);

    foreach(Box2DFixture *objectFixture, m_fixtures)
    {
        if(!objectFixture)
            continue;

        // Check to see if the user of the library set the x, y, width and/or height values.
        Box2DCircle *circle = static_cast<Box2DCircle *>(objectFixture);
        if(!circle)
            return;

        Box2DCircle *fixture = new Box2DCircle(item);

        fixture->setX(circle->x());
        fixture->setY(circle->y());

        if(circle->radius() == 0.0)
            fixture->setRadius(object.width() == 0.0 ? object.height() : object.width());
        else
            fixture->setRadius(circle->radius());

        fixture->setDensity(circle->density());
        fixture->setFriction(circle->friction());
        fixture->setRestitution(circle->restitution());
        fixture->setSensor(circle->isSensor());
        fixture->setCategories(circle->categories());
        fixture->setCollidesWith(circle->collidesWith());
        fixture->setGroupIndex(circle->groupIndex());

        connect(circle, SIGNAL(beginContact(Box2DFixture*)), fixture, SIGNAL(beginContact(Box2DFixture*)));
        connect(circle, SIGNAL(endContact(Box2DFixture*)), fixture, SIGNAL(endContact(Box2DFixture*)));

        body->addFixture(fixture);
    }
    body->componentComplete();
    item->setBody(body);
}

void TiledObject::createPolygonFixture(const TMXMapObject &object, CollisionItem *item)
{
    if(!item)
        return;
    if(m_fixtures.isEmpty())
        return;

    Box2DBody *body = new Box2DBody(item);
    body->setBodyType(Box2DBody::Static);
    body->setTarget(item);
    body->setActive(true);
    body->setSleepingAllowed(false);

    foreach(Box2DFixture *objectFixture, m_fixtures)
    {
        if(!objectFixture)
            continue;

        // Check to see if the user of the library set the x, y, width and/or height values.
        Box2DPolygon *polygon = static_cast<Box2DPolygon *>(objectFixture);
        if(!polygon)
            return;
        Box2DPolygon *fixture = new Box2DPolygon(item);

        QVariantList vertices = polygon->vertices().isEmpty() ? object.polygonAsList()
                                                                : polygon->vertices();
        fixture->setVertices(vertices);

        fixture->setDensity(polygon->density());
        fixture->setFriction(polygon->friction());
        fixture->setRestitution(polygon->restitution());
        fixture->setSensor(polygon->isSensor());
        fixture->setCategories(polygon->categories());
        fixture->setCollidesWith(polygon->collidesWith());
        fixture->setGroupIndex(polygon->groupIndex());

        connect(polygon, SIGNAL(beginContact(Box2DFixture*)), fixture, SIGNAL(beginContact(Box2DFixture*)));
        connect(polygon, SIGNAL(endContact(Box2DFixture*)), fixture, SIGNAL(endContact(Box2DFixture*)));

        body->addFixture(fixture);
    }
    body->componentComplete();
    item->setBody(body);
}

void TiledObject::createPolylineFixture(const TMXMapObject &object, CollisionItem *item)
{
    if(!item)
        return;
    if(m_fixtures.isEmpty())
        return;

    Box2DBody *body = new Box2DBody(item);
    body->setBodyType(Box2DBody::Static);
    body->setTarget(item);
    body->setActive(true);
    body->setSleepingAllowed(false);

    foreach(Box2DFixture *objectFixture, m_fixtures)
    {
        if(!objectFixture)
            continue;

        // Check to see if the user of the library set the x, y, width and/or height values.
        Box2DChain *chain = static_cast<Box2DChain *>(objectFixture);
        if(!chain)
            return;

        Box2DChain *fixture = new Box2DChain(item);

        QVariantList vertices = chain->vertices().isEmpty() ? object.polygonAsList()
                                                                : chain->vertices();
        fixture->setVertices(vertices);

        fixture->setDensity(chain->density());
        fixture->setFriction(chain->friction());
        fixture->setRestitution(chain->restitution());
        fixture->setSensor(chain->isSensor());
        fixture->setCategories(chain->categories());
        fixture->setCollidesWith(chain->collidesWith());
        fixture->setGroupIndex(chain->groupIndex());

        connect(chain, SIGNAL(beginContact(Box2DFixture*)), fixture, SIGNAL(beginContact(Box2DFixture*)));
        connect(chain, SIGNAL(endContact(Box2DFixture*)), fixture, SIGNAL(endContact(Box2DFixture*)));

        body->addFixture(fixture);
    }
    body->componentComplete();
    item->setBody(body);
}

bool TiledObject::next()
{
    return setCollisionIndex(m_collisionIndex + 1);
}

bool TiledObject::previous()
{
    return setCollisionIndex(m_collisionIndex - 1);
}

bool TiledObject::setCollisionIndex(int index)
{
    if(m_collisionIndex == index)
        return false;
    if(index < -1 || index > m_collisionItems.count())
    {
        qWarning() << "TiledObject: Collision index out of range.";
        return false;
    }
    if(index == -1 || index == m_collisionItems.count())
    {
        m_collisionIndex = index;
        emit collisionIndexChanged();
        return false;
    }

    if(index >= 0 && index < m_collisionItems.count())
    {
        CollisionItem *item = m_collisionItems[index];
        if(!item)
            return false;

        setProperties(item->properties());
        setX(item->x());
        setY(item->y());
        setWidth(item->width());
        setHeight(item->height());
        setRotation(item->rotation());

        setVisible(item->isVisible());
        setBody(item->body());
    }

    m_collisionIndex = index;
    emit collisionIndexChanged();

    return true;
}

QQmlListProperty<Box2DFixture> TiledObject::fixtures()
{
    return QQmlListProperty<Box2DFixture>(this, 0,
                                        &TiledObject::append_fixture,
                                        &TiledObject::count_fixture,
                                        &TiledObject::at_fixture,
                                        0);
}

void TiledObject::append_fixture(QQmlListProperty<Box2DFixture> *list, Box2DFixture *fixture)
{
    TiledObject *object = static_cast<TiledObject *>(list->object);
    fixture->setParent(object);
    object->m_fixtures.append(fixture);
}

int TiledObject::count_fixture(QQmlListProperty<Box2DFixture> *list)
{
    TiledObject *object = static_cast<TiledObject *>(list->object);
    return object->m_fixtures.length();
}

Box2DFixture *TiledObject::at_fixture(QQmlListProperty<Box2DFixture> *list, int index)
{
    TiledObject *object = static_cast<TiledObject *>(list->object);
    return object->m_fixtures.at(index);
}

void TiledObject::componentComplete() {
    QQuickItem::componentComplete();

    if(m_componentCompleted)
        initialize();
}

QQmlListProperty<CollisionItem> TiledObject::collisions()
{
    return QQmlListProperty<CollisionItem>(this, 0,
                                        &TiledObject::count_collision,
                                        &TiledObject::at_collision);
}

int TiledObject::count_collision(QQmlListProperty<CollisionItem> *list)
{
    TiledObject *object = static_cast<TiledObject *>(list->object);
    return object->m_collisionItems.length();
}

CollisionItem *TiledObject::at_collision(QQmlListProperty<CollisionItem> *list, int index)
{
    TiledObject *object = static_cast<TiledObject *>(list->object);
    return object->m_collisionItems.at(index);
}
