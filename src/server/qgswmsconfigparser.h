/***************************************************************************
                              qgswmsconfigparser.h
                              --------------------
  begin                : March 25, 2014
  copyright            : (C) 2014 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSWMSCONFIGPARSER_H
#define QGSWMSCONFIGPARSER_H

#include "qgsunittypes.h"
#include "qgspallabeling.h"
#include "qgsdatumtransformstore.h"
#include "qgis_server.h"

class QPainter;
class QDomDocument;
class QgsComposerHtml;
class QgsComposerLabel;
class QgsComposerLegend;
class QgsComposerMap;
class QgsComposition;
class QgsMapLayer;
class QgsLegendModel;
class QgsMapSettings;


class SERVER_EXPORT QgsWmsConfigParser
{
  public:

    QgsWmsConfigParser();
    virtual ~QgsWmsConfigParser() = default;

    /** Adds layer and style specific capabilities elements to the parent node. This includes the individual layers and styles, their description, native CRS, bounding boxes, etc.
        @param fullProjectInformation If true: add extended project information (does not validate against WMS schema)*/
    virtual void layersAndStylesCapabilities( QDomElement& parentElement, QDomDocument& doc, const QString& version, bool fullProjectSettings = false ) const = 0;

    //! Returns one or possibly several maplayers for a given layer name and style. If no layers/style are found, an empty list is returned
    virtual QList<QgsMapLayer*> mapLayerFromStyle( const QString& lName, const QString& styleName, bool useCache = true ) const = 0;

    //! Fills a layer and a style list. The two list have the same number of entries and the style and the layer at a position belong together (similar to the HTTP parameters 'Layers' and 'Styles'. Returns 0 in case of success
    virtual int layersAndStyles( QStringList& layers, QStringList& styles ) const = 0;

    //! Returns the xml fragment of a style
    virtual QDomDocument getStyle( const QString& styleName, const QString& layerName ) const = 0;

    //! Returns the xml fragment of layers styles
    virtual QDomDocument getStyles( QStringList& layerList ) const = 0;

    //! Returns the xml fragment of layers styles description
    virtual QDomDocument describeLayer( QStringList& layerList, const QString& hrefString ) const = 0;

    //! Returns if output are MM or PIXEL
    virtual QgsUnitTypes::RenderUnit outputUnits() const = 0;

    //! Returns an ID-list of layers which are not queryable (comes from <properties> -> <Identify> -> <disabledLayers in the project file
    virtual QStringList identifyDisabledLayers() const = 0;

    //! True if the feature info response should contain the wkt geometry for vector features
    virtual bool featureInfoWithWktGeometry() const = 0;

    //! True if the feature info wkt geometry is delivered with segmentized curve types
    virtual bool segmentizeFeatureInfoWktGeometry() const = 0;

    //! Returns map with layer aliases for GetFeatureInfo (or 0 pointer if not supported). Key: layer name, Value: layer alias
    virtual QHash<QString, QString> featureInfoLayerAliasMap() const = 0;

    virtual QString featureInfoDocumentElement( const QString& defaultValue ) const = 0;

    virtual QString featureInfoDocumentElementNS() const = 0;

    virtual QString featureInfoSchema() const = 0;

    //! Return feature info in format SIA2045?
    virtual bool featureInfoFormatSIA2045() const = 0;

    //! Draw text annotation items from the QGIS projectfile
    virtual void drawOverlays( QPainter* p, int dpi, int width, int height ) const = 0;

    //! Load PAL engine settings  into global project instance
    virtual void loadLabelSettings() const = 0;

    virtual QString serviceUrl() const = 0;

    virtual QStringList wfsLayerNames() const = 0;

    virtual void owsGeneralAndResourceList( QDomElement& parentElement, QDomDocument& doc, const QString& strHref ) const = 0;

    //legend
    virtual double legendBoxSpace() const = 0;
    virtual double legendLayerSpace() const = 0;
    virtual double legendLayerTitleSpace() const = 0;
    virtual double legendSymbolSpace() const = 0;
    virtual double legendIconLabelSpace() const = 0;
    virtual double legendSymbolWidth() const = 0;
    virtual double legendSymbolHeight() const = 0;
    virtual const QFont& legendLayerFont() const = 0;
    virtual const QFont& legendItemFont() const = 0;

    virtual double maxWidth() const = 0;
    virtual double maxHeight() const = 0;
    virtual double imageQuality() const = 0;

    // WMS GetFeatureInfo precision (decimal places)
    virtual int wmsPrecision() const = 0;

    // WMS inspire capabilities
    virtual bool wmsInspireActivated() const = 0;
    //! Adds inspire capabilities to xml document. ParentElem usually is the <Capabilities> element
    virtual void inspireCapabilities( QDomElement& parentElement, QDomDocument& doc ) const = 0;

    //printing
    QgsComposition* createPrintComposition( const QString& composerTemplate, const QgsMapSettings& mapSettings, const QMap< QString, QString >& parameterMap, QStringList& highlightLayers ) const;

    //! Creates a composition from the project file (probably delegated to the fallback parser)
    virtual QgsComposition* initComposition( const QString& composerTemplate, const QgsMapSettings& mapSettings, QList< QgsComposerMap*>& mapList, QList< QgsComposerLegend* >& legendList, QList< QgsComposerLabel* >& labelList, QList<const QgsComposerHtml *>& htmlFrameList ) const = 0;

    //! Adds print capabilities to xml document. ParentElem usually is the <Capabilities> element
    virtual void printCapabilities( QDomElement& parentElement, QDomDocument& doc ) const = 0;

    virtual void setScaleDenominator( double denom ) = 0;
    virtual void addExternalGMLData( const QString& layerName, QDomDocument* gmlDoc ) = 0;

    virtual QList< QPair< QString, QgsDatumTransformStore::Entry > > layerCoordinateTransforms() const = 0;

    virtual int nLayers() const = 0;

    virtual void serviceCapabilities( QDomElement& parentElement, QDomDocument& doc ) const = 0;

    virtual bool useLayerIds() const = 0;

    virtual bool allowRequestDefinedDatasources() const { return false; }

    //! Adds highlight layers to the layer registry and to the layer set. Returns the ids of the newly created layers (for later removal)
    static QStringList addHighlightLayers( const QMap<QString, QString>& parameterMap, QStringList& layerSet, const QString& parameterPrefix = QString() );
    static void removeHighlightLayers( const QStringList& layerIds );

#if 0
    //! List of GML datasets passed outside SLD (e.g. in a SOAP request). Key of the map is the layer name
    QMap<QString, QDomDocument*> mExternalGMLDatasets;
#endif //0

  private:

    //helper methods for 'addHighlightLayers'
    static void highlightParameters( const QMap<QString, QString>& parameterMap, const QString &parameterPrefix, QStringList& geom, QStringList& symbol, QStringList& label,
                                     QStringList& labelFont, QStringList& labelSize, QStringList& labelWeight, QStringList& labelColor,
                                     QStringList& labelBufferColor, QStringList &labelBufferSize );

    static QgsVectorLayer* createHighlightLayer( int i, const QString& crsString, QgsGeometry* geom, const QString& labelString, const QStringList& labelSizeSplit, const QStringList& labelColorSplit,
        const QStringList& labelWeightSplit, const QStringList& labelFontSplit, const QStringList& labelBufferSizeSplit,
        const QStringList& labelBufferColorSplit );

    static void setLayerIdsToLegendModel( QgsLegendModel* model, const QStringList& layerIds, double scale );
};

#endif // QGSWMSCONFIGPARSER_H
