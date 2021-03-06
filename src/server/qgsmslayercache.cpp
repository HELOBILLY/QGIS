/***************************************************************************
                              qgsmslayercache.cpp
                              -------------------
  begin                : September 22, 2007
  copyright            : (C) 2007 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmslayercache.h"
#include "qgsmessagelog.h"
#include "qgsproject.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"
#include <QFile>

QgsMSLayerCache* QgsMSLayerCache::instance()
{
  static QgsMSLayerCache *mInstance = 0;
  if ( !mInstance )
    mInstance = new QgsMSLayerCache();
  return mInstance;
}

QgsMSLayerCache::QgsMSLayerCache()
    : mProjectMaxLayers( 0 )
{
  mDefaultMaxLayers = 100;
  //max layer from environment variable overrides default
  char* maxLayerEnv = getenv( "MAX_CACHE_LAYERS" );
  if ( maxLayerEnv )
  {
    bool conversionOk = false;
    int maxLayerInt = QString( maxLayerEnv ).toInt( &conversionOk );
    if ( conversionOk )
    {
      mDefaultMaxLayers = maxLayerInt;
    }
  }
  QObject::connect( &mFileSystemWatcher, SIGNAL( fileChanged( const QString& ) ), this, SLOT( removeProjectFileLayers( const QString& ) ) );
}

QgsMSLayerCache::~QgsMSLayerCache()
{
  QgsDebugMsg( "removing all entries" );
  Q_FOREACH ( QgsMSLayerCacheEntry entry, mEntries )
  {
    delete entry.layerPointer;
  }
  mEntries.clear();
}

void QgsMSLayerCache::insertLayer( const QString& url, const QString& layerName, QgsMapLayer* layer, const QString& configFile, const QList<QString>& tempFiles )
{
  QgsMessageLog::logMessage( "Layer cache: insert Layer '" + layerName + "' configFile: " + configFile, QStringLiteral( "Server" ), QgsMessageLog::INFO );
  if ( mEntries.size() > qMax( mDefaultMaxLayers, mProjectMaxLayers ) ) //force cache layer examination after 10 inserted layers
  {
    updateEntries();
  }

  QPair<QString, QString> urlLayerPair = qMakePair( url, layerName );

  QgsMSLayerCacheEntry newEntry;
  newEntry.layerPointer = layer;
  newEntry.url = url;
  newEntry.creationTime = time( nullptr );
  newEntry.lastUsedTime = time( nullptr );
  newEntry.temporaryFiles = tempFiles;
  newEntry.configFile = configFile;

  mEntries.insert( urlLayerPair, newEntry );

  //update config file map
  if ( !configFile.isEmpty() )
  {
    QHash< QString, int >::const_iterator configIt = mConfigFiles.constFind( configFile );
    if ( configIt == mConfigFiles.constEnd() )
    {
      mConfigFiles.insert( configFile, 1 );
      mFileSystemWatcher.addPath( configFile );
    }
    else
    {
      mConfigFiles[configFile] = configIt.value() + 1; //increment reference counter
    }
  }
}

QgsMapLayer* QgsMSLayerCache::searchLayer( const QString& url, const QString& layerName, const QString& configFile )
{
  QPair<QString, QString> urlNamePair = qMakePair( url, layerName );
  if ( !mEntries.contains( urlNamePair ) )
  {
    QgsMessageLog::logMessage( "Layer '" + layerName + "' configFile: " + configFile + " not found in layer cache'", QStringLiteral( "Server" ), QgsMessageLog::INFO );
    return nullptr;
  }
  else
  {
    QList< QgsMSLayerCacheEntry > layers = mEntries.values( urlNamePair );
    QList< QgsMSLayerCacheEntry >::iterator layerIt = layers.begin();
    for ( ; layerIt != layers.end(); ++layerIt )
    {
      if ( configFile.isEmpty() || layerIt->configFile == configFile )
      {
        layerIt->lastUsedTime = time( nullptr );
        QgsMessageLog::logMessage( "Layer '" + layerName + "' configFile: " + configFile + " found in layer cache", QStringLiteral( "Server" ), QgsMessageLog::INFO );
        return layerIt->layerPointer;
      }
    }
    QgsMessageLog::logMessage( "Layer '" + layerName + "' configFile: " + configFile + " not found in layer cache'", QStringLiteral( "Server" ), QgsMessageLog::INFO );
    return nullptr;
  }
}

void QgsMSLayerCache::removeProjectFileLayers( const QString& project )
{
  QgsMessageLog::logMessage( "Removing cache entries for project file: " + project, QStringLiteral( "Server" ), QgsMessageLog::INFO );
  QVector< QPair< QString, QString > > removeEntries;
  QVector< QgsMSLayerCacheEntry > removeEntriesValues;

  QHash<QPair<QString, QString>, QgsMSLayerCacheEntry>::iterator entryIt = mEntries.begin();
  for ( ; entryIt != mEntries.end(); ++entryIt )
  {
    if ( entryIt.value().configFile == project )
    {
      removeEntries.push_back( entryIt.key() );
      removeEntriesValues.push_back( entryIt.value() );
      freeEntryResources( entryIt.value() );
    }
  }

  for ( int i = 0; i < removeEntries.size(); ++i )
  {
    const QgsMSLayerCacheEntry& removeEntry = removeEntriesValues.at( i );
    const QPair< QString, QString > removeKey = removeEntries.at( i );
    QgsMessageLog::logMessage( "Removing cache entry for url:" +  removeKey.first + " layerName:" + removeKey.second + " project file:" + project, QStringLiteral( "Server" ), QgsMessageLog::INFO );
    mEntries.remove( removeKey, removeEntry );
  }
}

void QgsMSLayerCache::updateEntries()
{
  QgsDebugMsg( "updateEntries" );
  int entriesToDelete = mEntries.size() - qMax( mDefaultMaxLayers, mProjectMaxLayers );
  if ( entriesToDelete < 1 )
  {
    return;
  }

  for ( int i = 0; i < entriesToDelete; ++i )
  {
    removeLeastUsedEntry();
  }
}

void QgsMSLayerCache::removeLeastUsedEntry()
{
  if ( mEntries.size() < 1 )
  {
    return;
  }

  QHash<QPair<QString, QString>, QgsMSLayerCacheEntry>::iterator it = mEntries.begin();
  QHash<QPair<QString, QString>, QgsMSLayerCacheEntry>::iterator lowest_it = it;
  time_t lowest_time = it->lastUsedTime;

  for ( ; it != mEntries.end(); ++it )
  {
    if ( it->lastUsedTime < lowest_time )
    {
      lowest_it = it;
      lowest_time = it->lastUsedTime;
    }
  }

  QgsMessageLog::logMessage( "Removing last accessed layer '" + lowest_it.value().layerPointer->name() + "' project file " + lowest_it.value().configFile + " from cache" , QStringLiteral( "Server" ), QgsMessageLog::INFO );
  freeEntryResources( *lowest_it );
  mEntries.erase( lowest_it );
}

void QgsMSLayerCache::freeEntryResources( QgsMSLayerCacheEntry& entry )
{
  // remove layer from QgsProject before delete it
  if ( QgsProject::instance()->mapLayer( entry.layerPointer->id() ) )
    QgsProject::instance()->removeMapLayer( entry.layerPointer->id() );

  delete entry.layerPointer;

  //remove the temporary files of a layer
  Q_FOREACH ( const QString& file, entry.temporaryFiles )
  {
    //remove the temporary file
    QFile removeFile( file );
    if ( !removeFile.remove() )
    {
      QgsDebugMsg( "could not remove file: " + file );
      QgsDebugMsg( removeFile.errorString() );
    }
  }

  //counter
  if ( !entry.configFile.isEmpty() )
  {
    int configFileCount = mConfigFiles[entry.configFile];
    if ( configFileCount < 2 )
    {
      mConfigFiles.remove( entry.configFile );
      mFileSystemWatcher.removePath( entry.configFile );
    }
    else
    {
      mConfigFiles[entry.configFile] = configFileCount - 1;
    }
  }
}

void QgsMSLayerCache::logCacheContents() const
{
  QgsMessageLog::logMessage( QStringLiteral( "Layer cache contents:" ) , QStringLiteral( "Server" ), QgsMessageLog::INFO );
  QHash<QPair<QString, QString>, QgsMSLayerCacheEntry>::const_iterator it = mEntries.constBegin();
  for ( ; it != mEntries.constEnd(); ++it )
  {
    QgsMessageLog::logMessage( "Url: " + it.value().url + " Layer name: " + it.value().layerPointer->name() + " Project: " + it.value().configFile, QStringLiteral( "Server" ), QgsMessageLog::INFO );
  }
}


void QgsMSLayerCache::removeProjectLayers( const QString& path )
{
  removeProjectFileLayers( path );
}
