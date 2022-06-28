/*
 * Copyright (C) 2020 Damir Porobic <damir.porobic@gmx.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "AnnotationTabWidget.h"

namespace kImageAnnotator {

AnnotationTabWidget::AnnotationTabWidget(Config *config, AbstractSettingsProvider *settingsProvider) :
	mConfig(config),
	mTabBar(tabBar()),
	mSettingsProvider(settingsProvider),
	mRedoAction(new QAction(this)),
	mUndoAction(new QAction(this)),
	mTabContextMenu(new AnnotationTabContextMenu(this)),
	mTabCloser(new AnnotationTabCloser(this)),
	mTabClickFilter(new AnnotationTabClickEventFilter(mTabBar, this)),
    mCanUndoRedoCompressor(new QTimer(this))
{
	setTabBarAutoHide(true);
	setMovable(true);
	setTabsClosable(true);
	mTabBar->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(mUndoAction, &QAction::triggered, this, &AnnotationTabWidget::undoTriggered);
	connect(mRedoAction, &QAction::triggered, this, &AnnotationTabWidget::redoTriggered);
	connect(mTabBar, &QTabBar::tabMoved, this, &AnnotationTabWidget::tabMoved);
	connect(mTabBar, &QTabBar::customContextMenuRequested, this, &AnnotationTabWidget::showTabContextMenu);
	connect(mTabBar, &QTabBar::currentChanged, this, &AnnotationTabWidget::tabChanged);

	connect(mTabContextMenu, &AnnotationTabContextMenu::closeTab, mTabCloser, &AnnotationTabCloser::closeTabTriggered);
	connect(mTabContextMenu, &AnnotationTabContextMenu::closeOtherTabs, mTabCloser, &AnnotationTabCloser::closeOtherTabsTriggered);
	connect(mTabContextMenu, &AnnotationTabContextMenu::closeAllTabs, mTabCloser, &AnnotationTabCloser::closeAllTabsTriggered);
	connect(mTabContextMenu, &AnnotationTabContextMenu::closeAllTabsToLeft, mTabCloser, &AnnotationTabCloser::closeAllTabsToLeftTriggered);
	connect(mTabContextMenu, &AnnotationTabContextMenu::closeAllTabsToRight, mTabCloser, &AnnotationTabCloser::closeAllTabsToRightTriggered);

	connect(mTabClickFilter, &AnnotationTabClickEventFilter::closeTabTriggered, mTabCloser, &AnnotationTabCloser::closeTabTriggered);
    
    mCanUndoRedoCompressor->setInterval(100);
    mCanUndoRedoCompressor->setSingleShot(true);
    connect(mCanUndoRedoCompressor, &QTimer::timeout, this, &AnnotationTabWidget::updateUndoRedoEnabled);
}

int AnnotationTabWidget::addTab(const QPixmap &image, const QString &title, const QString &toolTip)
{
	auto content = new AnnotationTabContent(image, mConfig, mSettingsProvider);
    auto annotationArea = content->annotationArea();
	connect(annotationArea, &AnnotationArea::imageChanged, this, &AnnotationTabWidget::imageChanged);
    connect(annotationArea, &AnnotationArea::canUndoChanged, mCanUndoRedoCompressor, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(annotationArea, &AnnotationArea::canRedoChanged, mCanUndoRedoCompressor, static_cast<void (QTimer::*)()>(&QTimer::start));

	return QTabWidget::addTab(content, title);
}

AnnotationArea* AnnotationTabWidget::currentAnnotationArea() const
{
	return currentWidget() != nullptr ? dynamic_cast<AnnotationTabContent*>(currentWidget())->annotationArea() : nullptr;
}

AnnotationArea *AnnotationTabWidget::annotationAreaAt(int index) const
{
	return widget(index) != nullptr ? dynamic_cast<AnnotationTabContent*>(widget(index))->annotationArea() : nullptr;
}

ZoomValueProvider *AnnotationTabWidget::currentZoomValueProvider() const
{
	return currentWidget() != nullptr ? dynamic_cast<AnnotationTabContent *>(currentWidget())->zoomValueProvider() : nullptr;
}

QAction *AnnotationTabWidget::undoAction() const
{
	return mUndoAction;
}

QAction *AnnotationTabWidget::redoAction() const
{
	return mRedoAction;
}

void AnnotationTabWidget::updateTabInfo(int index, const QString &title, const QString &toolTip)
{
	setTabText(index, title);
	setTabToolTip(index, toolTip);
}

void AnnotationTabWidget::tabInserted(int index)
{
	updateCurrentWidget(index);
	QTabWidget::tabInserted(index);
}

void AnnotationTabWidget::tabRemoved(int index)
{
	QTabWidget::tabRemoved(index);
}

void AnnotationTabWidget::undoTriggered() const
{
	auto annotationArea = currentAnnotationArea();
	if(annotationArea != nullptr) {
		annotationArea->undoAction()->trigger();
	}
}

void AnnotationTabWidget::redoTriggered() const
{
	auto annotationArea = currentAnnotationArea();
	if(annotationArea != nullptr) {
		annotationArea->redoAction()->trigger();
	}
}

void AnnotationTabWidget::updateCurrentWidget(int index)
{
	setCurrentIndex(index);
}

void AnnotationTabWidget::showTabContextMenu(const QPoint &pos)
{
	if (!pos.isNull()) {
		int tabIndex = mTabBar->tabAt(pos);
		emit tabContextMenuOpened(tabIndex);
		mTabContextMenu->show(tabIndex, mTabBar->mapToGlobal(pos));
	}
}

void AnnotationTabWidget::tabChanged()
{
	mSettingsProvider->setActiveListener(currentAnnotationArea());
	mSettingsProvider->setActiveZoomValueProvider(currentZoomValueProvider());
    updateUndoRedoEnabled();
}

void AnnotationTabWidget::updateUndoRedoEnabled()
{
    auto curAnnotationArea = currentAnnotationArea();
    const bool undoEnabled = curAnnotationArea && curAnnotationArea->canUndo();
    if (mUndoAction->isEnabled() != undoEnabled) {
        mUndoAction->setEnabled(undoEnabled);
        emit canUndoChanged(undoEnabled);
    }
    const bool redoEnabled = curAnnotationArea && curAnnotationArea->canRedo();
    if (mRedoAction->isEnabled() != redoEnabled) {
        mRedoAction->setEnabled(redoEnabled);
        emit canRedoChanged(redoEnabled);
    }
}

void AnnotationTabWidget::addContextMenuActions(const QList<QAction *> &actions)
{
	mTabContextMenu->addCustomActions(actions);
}

} // namespace kImageAnnotator