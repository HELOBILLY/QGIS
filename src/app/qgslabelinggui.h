/***************************************************************************
  qgslabelinggui.h
  Smart labeling for vector layers
  -------------------
         begin                : June 2009
         copyright            : (C) Martin Dobias
         email                : wonder dot sk at gmail dot com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSLABELINGGUI_H
#define QGSLABELINGGUI_H

#include "qgspallabeling.h"
#include "qgstextformatwidget.h"
#include "qgis_app.h"

class APP_EXPORT QgsLabelingGui : public QgsTextFormatWidget, private QgsExpressionContextGenerator
{
    Q_OBJECT

  public:
    QgsLabelingGui( QgsVectorLayer* layer, QgsMapCanvas* mapCanvas, const QgsPalLayerSettings* settings, QWidget* parent );

    QgsPalLayerSettings layerSettings();
    void writeSettingsToLayer();

    enum LabelMode
    {
      NoLabels,
      Labels,
      ObstaclesOnly,
    };

    void setLabelMode( LabelMode mode );

    void setLayer( QgsMapLayer* layer );

  public slots:

    void apply();

    void updateUi();

  protected:
    void blockInitSignals( bool block );
    void syncDefinedCheckboxFrame( QgsDataDefinedButton* ddBtn, QCheckBox* chkBx, QFrame* f );
    void populateDataDefinedButtons( QgsPalLayerSettings& s );
    //! Sets data defined property attribute to map
    void setDataDefinedProperty( const QgsDataDefinedButton* ddBtn, QgsPalLayerSettings::DataDefinedProperties p, QgsPalLayerSettings& lyr );

  private:
    QgsVectorLayer* mLayer;
    const QgsPalLayerSettings* mSettings;
    LabelMode mMode;

    QgsExpressionContext createExpressionContext() const override;

};

#endif // QGSLABELINGGUI_H


