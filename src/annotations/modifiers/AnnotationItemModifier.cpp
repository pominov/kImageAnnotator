/*
 * Copyright (C) 2018 Damir Porobic <damir.porobic@gmx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "AnnotationItemModifier.h"

AnnotationItemModifier::AnnotationItemModifier()
{
    mItemSelector = new AnnotationItemSelector();
    mItemResizer = new AnnotationMultiItemResizer();
    mItemMover = new AnnotationItemMover();
    addToGroup(mItemSelector);
    addToGroup(mItemResizer);
    setZValue(1000);
    setAcceptHoverEvents(true);
}

AnnotationItemModifier::~AnnotationItemModifier()
{
    delete mItemResizer;
    delete mItemSelector;
    delete mItemMover;
}

void AnnotationItemModifier::handleMousePress(const QPointF& pos, QList<AbstractAnnotationItem*>* items, bool isCtrlPressed)
{
    mItemResizer->grabHandle(pos);
    if(mItemResizer->isResizing()) {
        return;
    }

    mItemSelector->handleSelectionAt(pos, items, isCtrlPressed);
    if(mItemSelector->isSelecting()) {
        mItemResizer->detach();
        return;
    }

    auto selectedItems = mItemSelector->selectedItems();
    mItemMover->setOffset(pos, selectedItems);

    handleSelection();
    updateCursor(mItemMover->cursor());
}

void AnnotationItemModifier::handleMouseMove(const QPointF& pos)
{
    if(mItemResizer->isResizing()) {
        mItemResizer->moveHandle(pos);
    } else if(mItemSelector->isSelecting()) {
        mItemSelector->extendSelectionRectWhenShown(pos);
    } else {
        mItemMover->moveItems(pos);
        mItemResizer->refresh();
        mItemSelector->refresh();
    }
}

void AnnotationItemModifier::handleMouseRelease(QList<AbstractAnnotationItem*>* items)
{
    if(mItemResizer->isResizing()) {
        mItemResizer->releaseHandle();
    } else if(mItemSelector->isSelecting()) {
        mItemSelector->finishSelectionRectWhenShown(items);
    } else {
        mItemMover->clearOffset();
        updateCursor(mItemMover->cursor());
    }

    handleSelection();
}

void AnnotationItemModifier::handleSelectionAt(const QPointF& pos, QList<AbstractAnnotationItem *>* items, bool isCtrlPressed)
{
    mItemSelector->handleSelectionAt(pos, items, isCtrlPressed);
    handleSelection();
}

QList<AbstractAnnotationItem*> AnnotationItemModifier::selectedItems() const
{
    return mItemSelector->selectedItems();
}

QRectF AnnotationItemModifier::boundingRect() const
{
    if(mItemResizer->hasItemsAttached()) {
        return mItemResizer->boundingRect();
    }
    return mItemSelector->boundingRect();
}

void AnnotationItemModifier::clearSelection()
{
    mItemSelector->clearSelection();
    mItemResizer->detach();
}

void AnnotationItemModifier::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event)
    // Move Cursor disappears when we let this event propagate
}

void AnnotationItemModifier::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if(mItemMover->isMoving()) {
        return;
    }

    updateCursor(mItemResizer->cursor(event->scenePos()));
}

void AnnotationItemModifier::handleSelection()
{
    auto selectedItems = mItemSelector->selectedItems();
    auto count = selectedItems.count();
    if(count == 0) {
        clearSelection();
    } else {
        mItemResizer->attachTo(selectedItems);
    }
}

void AnnotationItemModifier::updateCursor(Qt::CursorShape cursor)
{
    if(cursor == CursorHelper::defaultCursor()) {
        unsetCursor();
    } else {
        setCursor(cursor);
    }
}
