/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoLibrary.h"
#include "messaging/ApplicationMessenger.h"
#include "TextureDatabase.h"
#include "Util.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

using namespace JSONRPC;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CVideoLibrary::GetMovies(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString("videodb://movies/titles/"))
    return InternalError;

  int genreID = -1, year = -1, setID = 0;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    videoUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("year"))
    year = (int)filter["year"].asInteger();
  else if (filter.isMember("actor"))
    videoUrl.AddOption("actor", filter["actor"].asString());
  else if (filter.isMember("director"))
    videoUrl.AddOption("director", filter["director"].asString());
  else if (filter.isMember("studio"))
    videoUrl.AddOption("studio", filter["studio"].asString());
  else if (filter.isMember("country"))
    videoUrl.AddOption("country", filter["country"].asString());
  else if (filter.isMember("setid"))
    setID = (int)filter["setid"].asInteger();
  else if (filter.isMember("set"))
    videoUrl.AddOption("set", filter["set"].asString());
  else if (filter.isMember("tag"))
    videoUrl.AddOption("tag", filter["tag"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("movies", filter, xsp))
      return InvalidParams;

    videoUrl.AddOption("xsp", xsp);
  }

  // setID must not be -1 otherwise GetMoviesNav() will return sets
  if (setID < 0)
    setID = 0;

  CFileItemList items;
  if (!videodatabase.GetMoviesNav(videoUrl.ToString(), items, genreID, year, -1, -1, -1, -1, setID, -1, sorting))
    return InvalidParams;

  return GetAdditionalMovieDetails(parameterObject, items, result, videodatabase, false);
}

JSONRPC_STATUS CVideoLibrary::GetMovieDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["movieid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetMovieInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  HandleFileItem("movieid", true, "moviedetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetMovieSets(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!videodatabase.GetSetsNav("videodb://movies/sets/", items, VIDEODB_CONTENT_MOVIES))
    return InternalError;

  HandleFileItemList("setid", false, "sets", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetMovieSetDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["setid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  // Get movie set details
  CVideoInfoTag infos;
  if (!videodatabase.GetSetInfo(id, infos) || infos.m_iDbId <= 0)
    return InvalidParams;

  HandleFileItem("setid", false, "setdetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);

  // Get movies from the set
  CFileItemList items;
  if (!videodatabase.GetMoviesNav("videodb://movies/titles/", items, -1, -1, -1, -1, -1, -1, id))
    return InternalError;

  return GetAdditionalMovieDetails(parameterObject["movies"], items, result["setdetails"], videodatabase, true);
}

JSONRPC_STATUS CVideoLibrary::GetTVShows(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString("videodb://tvshows/titles/"))
    return InternalError;

  int genreID = -1, year = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    videoUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("year"))
    year = (int)filter["year"].asInteger();
  else if (filter.isMember("actor"))
    videoUrl.AddOption("actor", filter["actor"].asString());
  else if (filter.isMember("studio"))
    videoUrl.AddOption("studio", filter["studio"].asString());
  else if (filter.isMember("tag"))
    videoUrl.AddOption("tag", filter["tag"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("tvshows", filter, xsp))
      return InvalidParams;

    videoUrl.AddOption("xsp", xsp);
  }

  CFileItemList items;
  if (!videodatabase.GetTvShowsNav(videoUrl.ToString(), items, genreID, year, -1, -1, -1, -1, sorting))
    return InvalidParams;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    std::string fieldValue = itr->asString();
    if (fieldValue == "cast" || fieldValue == "tag" || fieldValue == "file")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
      videodatabase.GetTvShowInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
  }

  int size = items.Size();
  if (items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("tvshowid", true, "tvshows", items, parameterObject, result, size, false);

  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetTVShowDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int id = (int)parameterObject["tvshowid"].asInteger();

  CFileItemPtr fileItem(new CFileItem());
  CVideoInfoTag infos;
  if (!videodatabase.GetTvShowInfo("", infos, id, fileItem.get()) || infos.m_iDbId <= 0)
    return InvalidParams;

  fileItem->SetFromVideoInfoTag(infos);
  HandleFileItem("tvshowid", true, "tvshowdetails", fileItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetSeasons(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int tvshowID = (int)parameterObject["tvshowid"].asInteger();

  std::string strPath = StringUtils::Format("videodb://tvshows/titles/%i/", tvshowID);
  CFileItemList items;
  if (!videodatabase.GetSeasonsNav(strPath, items, -1, -1, -1, -1, tvshowID, false))
    return InternalError;

  HandleFileItemList("seasonid", false, "seasons", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetSeasonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int id = (int)parameterObject["seasonid"].asInteger();

  CVideoInfoTag infos;
  if (!videodatabase.GetSeasonInfo(id, infos) ||
      infos.m_iDbId <= 0 || infos.m_iIdShow <= 0)
    return InvalidParams;
  
  CFileItemPtr pItem = CFileItemPtr(new CFileItem(infos));
  HandleFileItem("seasonid", false, "seasondetails", pItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetEpisodes(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  int tvshowID = (int)parameterObject["tvshowid"].asInteger();
  int season   = (int)parameterObject["season"].asInteger();
  
  std::string strPath = StringUtils::Format("videodb://tvshows/titles/%i/%i/", tvshowID, season);

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strPath))
    return InternalError;

  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("genreid"))
    videoUrl.AddOption("genreid", (int)filter["genreid"].asInteger());
  else if (filter.isMember("genre"))
    videoUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("year"))
    videoUrl.AddOption("year", (int)filter["year"].asInteger());
  else if (filter.isMember("actor"))
    videoUrl.AddOption("actor", filter["actor"].asString());
  else if (filter.isMember("director"))
    videoUrl.AddOption("director", filter["director"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("episodes", filter, xsp))
      return InvalidParams;

    videoUrl.AddOption("xsp", xsp);
  }

  if (tvshowID <= 0 && (season > 0 || videoUrl.HasOption("genreid") || videoUrl.HasOption("genre") || videoUrl.HasOption("actor")))
    return InvalidParams;

  if (tvshowID > 0)
  {
    videoUrl.AddOption("tvshowid", tvshowID);
    if (season >= 0)
      videoUrl.AddOption("season", season);
  }

  CFileItemList items;
  if (!videodatabase.GetEpisodesByWhere(videoUrl.ToString(), CDatabase::Filter(), items, false, sorting))
    return InvalidParams;

  return GetAdditionalEpisodeDetails(parameterObject, items, result, videodatabase, false);
}

JSONRPC_STATUS CVideoLibrary::GetEpisodeDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int id = (int)parameterObject["episodeid"].asInteger();

  CVideoInfoTag infos;
  if (!videodatabase.GetEpisodeInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  CFileItemPtr pItem = CFileItemPtr(new CFileItem(infos));
  // We need to set the correct base path to get the valid fanart
  int tvshowid = infos.m_iIdShow;
  if (tvshowid <= 0)
    tvshowid = videodatabase.GetTvShowForEpisode(id);

  std::string basePath = StringUtils::Format("videodb://tvshows/titles/%i/%i/%i", tvshowid, infos.m_iSeason, id);
  pItem->SetPath(basePath);

  HandleFileItem("episodeid", true, "episodedetails", pItem, parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetMusicVideos(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  SortDescription sorting;
  ParseLimits(parameterObject, sorting.limitStart, sorting.limitEnd);
  if (!ParseSorting(parameterObject, sorting.sortBy, sorting.sortOrder, sorting.sortAttributes))
    return InvalidParams;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString("videodb://musicvideos/titles/"))
    return InternalError;

  int genreID = -1, year = -1;
  const CVariant &filter = parameterObject["filter"];
  if (filter.isMember("artist"))
    videoUrl.AddOption("artist", filter["artist"].asString());
  else if (filter.isMember("genreid"))
    genreID = (int)filter["genreid"].asInteger();
  else if (filter.isMember("genre"))
    videoUrl.AddOption("genre", filter["genre"].asString());
  else if (filter.isMember("year"))
    year = (int)filter["year"].asInteger();
  else if (filter.isMember("director"))
    videoUrl.AddOption("director", filter["director"].asString());
  else if (filter.isMember("studio"))
    videoUrl.AddOption("studio", filter["studio"].asString());
  else if (filter.isMember("tag"))
    videoUrl.AddOption("tag", filter["tag"].asString());
  else if (filter.isObject())
  {
    std::string xsp;
    if (!GetXspFiltering("musicvideos", filter, xsp))
      return InvalidParams;

    videoUrl.AddOption("xsp", xsp);
  }

  CFileItemList items;
  if (!videodatabase.GetMusicVideosNav(videoUrl.ToString(), items, genreID, year, -1, -1, -1, -1, -1, sorting))
    return InternalError;

  return GetAdditionalMusicVideoDetails(parameterObject, items, result, videodatabase, false);
}

JSONRPC_STATUS CVideoLibrary::GetMusicVideoDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  int id = (int)parameterObject["musicvideoid"].asInteger();

  CVideoInfoTag infos;
  if (!videodatabase.GetMusicVideoInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  HandleFileItem("musicvideoid", true, "musicvideodetails", CFileItemPtr(new CFileItem(infos)), parameterObject, parameterObject["properties"], result, false);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedMovies(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!videodatabase.GetRecentlyAddedMoviesNav("videodb://recentlyaddedmovies/", items))
    return InternalError;

  return GetAdditionalMovieDetails(parameterObject, items, result, videodatabase, true);
}

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedEpisodes(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!videodatabase.GetRecentlyAddedEpisodesNav("videodb://recentlyaddedepisodes/", items))
    return InternalError;

  return GetAdditionalEpisodeDetails(parameterObject, items, result, videodatabase, true);
}

JSONRPC_STATUS CVideoLibrary::GetRecentlyAddedMusicVideos(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!videodatabase.GetRecentlyAddedMusicVideosNav("videodb://recentlyaddedmusicvideos/", items))
    return InternalError;

  return GetAdditionalMusicVideoDetails(parameterObject, items, result, videodatabase, true);
}

JSONRPC_STATUS CVideoLibrary::GetGenres(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string media = parameterObject["type"].asString();
  StringUtils::ToLower(media);
  int idContent = -1;

  std::string strPath = "videodb://";
  /* select which video content to get genres from*/
  if (media == MediaTypeMovie)
  {
    idContent = VIDEODB_CONTENT_MOVIES;
    strPath += "movies";
  }
  else if (media == MediaTypeTvShow)
  {
    idContent = VIDEODB_CONTENT_TVSHOWS;
    strPath += "tvshows";
  }
  else if (media == MediaTypeMusicVideo)
  {
    idContent = VIDEODB_CONTENT_MUSICVIDEOS;
    strPath += "musicvideos";
  }
  strPath += "/genres/";
 
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemList items;
  if (!videodatabase.GetGenresNav(strPath, items, idContent))
    return InternalError;

  /* need to set strTitle in each item*/
  for (unsigned int i = 0; i < (unsigned int)items.Size(); i++)
    items[i]->GetVideoInfoTag()->m_strTitle = items[i]->GetLabel();

  HandleFileItemList("genreid", false, "genres", items, parameterObject, result);
  return OK;
}

JSONRPC_STATUS CVideoLibrary::SetMovieDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["movieid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetMovieInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  if (videodatabase.UpdateDetailsForMovie(id, infos, artwork, updatedDetails) <= 0)
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, MediaTypeMovie, removedArtwork))
    return InternalError;

  if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
  {
    // restore original playcount or the new one won't be announced
    int newPlaycount = infos.m_playCount;
    infos.m_playCount = playcount;
    videodatabase.SetPlayCount(CFileItem(infos), newPlaycount, infos.m_lastPlayed);
  }

  UpdateResumePoint(parameterObject, infos, videodatabase);

  CJSONRPCUtils::NotifyItemUpdated(infos);
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::SetMovieSetDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["setid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetSetInfo(id, infos);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  if (videodatabase.SetDetailsForMovieSet(infos, artwork, id) <= 0)
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, "set", removedArtwork))
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::SetTVShowDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["tvshowid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetTvShowInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  std::map<int, std::map<std::string, std::string> > seasonArt;
  videodatabase.GetTvShowSeasonArt(infos.m_iDbId, seasonArt);

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  // we need to manually remove tags/taglinks for now because they aren't replaced
  // due to scrapers not supporting them
  videodatabase.RemoveTagsFromItem(id, MediaTypeTvShow);

  if (!videodatabase.UpdateDetailsForTvShow(id, infos, artwork, seasonArt))
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, MediaTypeTvShow, removedArtwork))
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::SetSeasonDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["seasonid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetSeasonInfo(id, infos);
  if (infos.m_iDbId <= 0 || infos.m_iIdShow <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  if (videodatabase.SetDetailsForSeason(infos, artwork, infos.m_iIdShow, id) <= 0)
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, MediaTypeSeason, removedArtwork))
    return InternalError;

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::SetEpisodeDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["episodeid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetEpisodeInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  int tvshowid = videodatabase.GetTvShowForEpisode(id);
  if (tvshowid <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  if (videodatabase.SetDetailsForEpisode(infos.m_strFileNameAndPath, infos, artwork, tvshowid, id) <= 0)
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, MediaTypeEpisode, removedArtwork))
    return InternalError;

  if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
  {
    // restore original playcount or the new one won't be announced
    int newPlaycount = infos.m_playCount;
    infos.m_playCount = playcount;
    videodatabase.SetPlayCount(CFileItem(infos), newPlaycount, infos.m_lastPlayed);
  }

  UpdateResumePoint(parameterObject, infos, videodatabase);

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::SetMusicVideoDetails(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["musicvideoid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  videodatabase.GetMusicVideoInfo("", infos, id);
  if (infos.m_iDbId <= 0)
  {
    videodatabase.Close();
    return InvalidParams;
  }

  // get artwork
  std::map<std::string, std::string> artwork;
  videodatabase.GetArtForItem(infos.m_iDbId, infos.m_type, artwork);

  int playcount = infos.m_playCount;
  CDateTime lastPlayed = infos.m_lastPlayed;

  std::set<std::string> removedArtwork;
  std::set<std::string> updatedDetails;
  UpdateVideoTag(parameterObject, infos, artwork, removedArtwork, updatedDetails);

  // we need to manually remove tags/taglinks for now because they aren't replaced
  // due to scrapers not supporting them
  videodatabase.RemoveTagsFromItem(id, MediaTypeMusicVideo);

  if (videodatabase.SetDetailsForMusicVideo(infos.m_strFileNameAndPath, infos, artwork, id) <= 0)
    return InternalError;

  if (!videodatabase.RemoveArtForItem(infos.m_iDbId, MediaTypeMusicVideo, removedArtwork))
    return InternalError;

  if (playcount != infos.m_playCount || lastPlayed != infos.m_lastPlayed)
  {
    // restore original playcount or the new one won't be announced
    int newPlaycount = infos.m_playCount;
    infos.m_playCount = playcount;
    videodatabase.SetPlayCount(CFileItem(infos), newPlaycount, infos.m_lastPlayed);
  }

  UpdateResumePoint(parameterObject, infos, videodatabase);

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::RefreshMovie(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = static_cast<int>(parameterObject["movieid"].asInteger());

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetMovieInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  bool ignoreNfo = parameterObject["ignorenfo"].asBoolean();
  std::string searchTitle = parameterObject["title"].asString();
  CVideoLibraryQueue::GetInstance().RefreshItem(CFileItemPtr(new CFileItem(infos)), ignoreNfo, true, false, searchTitle);

  return ACK;
}

JSONRPC_STATUS CVideoLibrary::RefreshTVShow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = static_cast<int>(parameterObject["tvshowid"].asInteger());

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CFileItemPtr item(new CFileItem());
  CVideoInfoTag infos;
  if (!videodatabase.GetTvShowInfo("", infos, id, item.get()) || infos.m_iDbId <= 0)
    return InvalidParams;

  item->SetFromVideoInfoTag(infos);

  bool ignoreNfo = parameterObject["ignorenfo"].asBoolean();
  bool refreshEpisodes = parameterObject["refreshepisodes"].asBoolean();
  std::string searchTitle = parameterObject["title"].asString();
  CVideoLibraryQueue::GetInstance().RefreshItem(item, ignoreNfo, true, refreshEpisodes, searchTitle);

  return ACK;
}

JSONRPC_STATUS CVideoLibrary::RefreshEpisode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = (int)parameterObject["episodeid"].asInteger();

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetEpisodeInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  CFileItemPtr item = CFileItemPtr(new CFileItem(infos));
  // We need to set the correct base path to get the valid fanart
  int tvshowid = infos.m_iIdShow;
  if (tvshowid <= 0)
    tvshowid = videodatabase.GetTvShowForEpisode(id);

  bool ignoreNfo = parameterObject["ignorenfo"].asBoolean();
  std::string searchTitle = parameterObject["title"].asString();
  CVideoLibraryQueue::GetInstance().RefreshItem(item, ignoreNfo, true, false, searchTitle);

  return ACK;
}

JSONRPC_STATUS CVideoLibrary::RefreshMusicVideo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  int id = static_cast<int>(parameterObject["musicvideoid"].asInteger());

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  CVideoInfoTag infos;
  if (!videodatabase.GetMusicVideoInfo("", infos, id) || infos.m_iDbId <= 0)
    return InvalidParams;

  bool ignoreNfo = parameterObject["ignorenfo"].asBoolean();
  std::string searchTitle = parameterObject["title"].asString();
  CVideoLibraryQueue::GetInstance().RefreshItem(CFileItemPtr(new CFileItem(infos)), ignoreNfo, true, false, searchTitle);

  return ACK;
}

JSONRPC_STATUS CVideoLibrary::RemoveMovie(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveTVShow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveEpisode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::RemoveMusicVideo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return RemoveVideo(parameterObject);
}

JSONRPC_STATUS CVideoLibrary::Scan(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string directory = parameterObject["directory"].asString();
  std::string cmd = StringUtils::Format("updatelibrary(video, %s, %s)", StringUtils::Paramify(directory).c_str(), parameterObject["showdialogs"].asBoolean() ? "true" : "false");

  CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::Export(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string cmd;
  if (parameterObject["options"].isMember("path"))
    cmd = StringUtils::Format("exportlibrary(video, false, %s)", StringUtils::Paramify(parameterObject["options"]["path"].asString()).c_str());
  else
    cmd = StringUtils::Format("exportlibrary(video, true, %s, %s, %s)",
                              parameterObject["options"]["images"].asBoolean() ? "true" : "false",
                              parameterObject["options"]["overwrite"].asBoolean() ? "true" : "false",
                              parameterObject["options"]["actorthumbs"].asBoolean() ? "true" : "false");

  CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

JSONRPC_STATUS CVideoLibrary::Clean(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  std::string cmd = StringUtils::Format("cleanlibrary(video, %s)", parameterObject["showdialogs"].asBoolean() ? "true" : "false");
  CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
  return ACK;
}

bool CVideoLibrary::FillFileItem(const std::string &strFilename, CFileItemPtr &item, const CVariant &parameterObject /* = CVariant(CVariant::VariantTypeArray) */)
{
  CVideoDatabase videodatabase;
  if (strFilename.empty())
    return false;
  
  bool filled = false;
  if (videodatabase.Open())
  {
    CVideoInfoTag details;
    if (videodatabase.LoadVideoInfo(strFilename, details))
    {
      item->SetFromVideoInfoTag(details);
      filled = true;
    }
  }

  if (item->GetLabel().empty())
  {
    item->SetLabel(CUtil::GetTitleFromPath(strFilename, false));
    if (item->GetLabel().empty())
      item->SetLabel(URIUtils::GetFileName(strFilename));
  }

  return filled;
}

bool CVideoLibrary::FillFileItemList(const CVariant &parameterObject, CFileItemList &list)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  std::string file = parameterObject["file"].asString();
  int movieID = (int)parameterObject["movieid"].asInteger(-1);
  int episodeID = (int)parameterObject["episodeid"].asInteger(-1);
  int musicVideoID = (int)parameterObject["musicvideoid"].asInteger(-1);

  bool success = false;
  CFileItemPtr fileItem(new CFileItem());
  if (FillFileItem(file, fileItem))
  {
    success = true;
    list.Add(fileItem);
  }

  if (movieID > 0)
  {
    CVideoInfoTag details;
    videodatabase.GetMovieInfo("", details, movieID);
    if (!details.IsEmpty())
    {
      list.Add(CFileItemPtr(new CFileItem(details)));
      success = true;
    }
  }
  if (episodeID > 0)
  {
    CVideoInfoTag details;
    if (videodatabase.GetEpisodeInfo("", details, episodeID) && !details.IsEmpty())
    {
      list.Add(CFileItemPtr(new CFileItem(details)));
      success = true;
    }
  }
  if (musicVideoID > 0)
  {
    CVideoInfoTag details;
    videodatabase.GetMusicVideoInfo("", details, musicVideoID);
    if (!details.IsEmpty())
    {
      list.Add(CFileItemPtr(new CFileItem(details)));
      success = true;
    }
  }

  return success;
}

JSONRPC_STATUS CVideoLibrary::GetAdditionalMovieDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase, bool limit /* = true */)
{
  if (!videodatabase.Open())
    return InternalError;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    std::string fieldValue = itr->asString();
    if (fieldValue == "cast" || fieldValue == "showlink" || fieldValue == "tag" || fieldValue == "streamdetails")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
    {
      if (additionalInfo)
        videodatabase.GetMovieInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
    }
  }

  int size = items.Size();
  if (!limit && items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("movieid", true, "movies", items, parameterObject, result, size, limit);

  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetAdditionalEpisodeDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase, bool limit /* = true */)
{
  if (!videodatabase.Open())
    return InternalError;

  bool additionalInfo = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    std::string fieldValue = itr->asString();
    if (fieldValue == "cast" || fieldValue == "streamdetails")
      additionalInfo = true;
  }

  if (additionalInfo)
  {
    for (int index = 0; index < items.Size(); index++)
    {
      if (additionalInfo)
        videodatabase.GetEpisodeInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
    }
  }
  
  int size = items.Size();
  if (!limit && items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("episodeid", true, "episodes", items, parameterObject, result, size, limit);

  return OK;
}

JSONRPC_STATUS CVideoLibrary::GetAdditionalMusicVideoDetails(const CVariant &parameterObject, CFileItemList &items, CVariant &result, CVideoDatabase &videodatabase, bool limit /* = true */)
{
  if (!videodatabase.Open())
    return InternalError;

  bool streamdetails = false;
  for (CVariant::const_iterator_array itr = parameterObject["properties"].begin_array(); itr != parameterObject["properties"].end_array(); itr++)
  {
    if (itr->asString() == "tag" || itr->asString() == "streamdetails")
      streamdetails = true;
  }

  if (streamdetails)
  {
    for (int index = 0; index < items.Size(); index++)
      videodatabase.GetMusicVideoInfo("", *(items[index]->GetVideoInfoTag()), items[index]->GetVideoInfoTag()->m_iDbId);
  }

  int size = items.Size();
  if (!limit && items.HasProperty("total") && items.GetProperty("total").asInteger() > size)
    size = (int)items.GetProperty("total").asInteger();
  HandleFileItemList("musicvideoid", true, "musicvideos", items, parameterObject, result, size, limit);

  return OK;
}

JSONRPC_STATUS CVideoLibrary::RemoveVideo(const CVariant &parameterObject)
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return InternalError;

  if (parameterObject.isMember("movieid"))
    videodatabase.DeleteMovie((int)parameterObject["movieid"].asInteger());
  else if (parameterObject.isMember("tvshowid"))
    videodatabase.DeleteTvShow((int)parameterObject["tvshowid"].asInteger());
  else if (parameterObject.isMember("episodeid"))
    videodatabase.DeleteEpisode((int)parameterObject["episodeid"].asInteger());
  else if (parameterObject.isMember("musicvideoid"))
    videodatabase.DeleteMusicVideo((int)parameterObject["musicvideoid"].asInteger());

  CJSONRPCUtils::NotifyItemUpdated();
  return ACK;
}

void CVideoLibrary::UpdateResumePoint(const CVariant &parameterObject, CVideoInfoTag &details, CVideoDatabase &videodatabase)
{
  if (!parameterObject["resume"].isNull())
  {
    int position = (int)parameterObject["resume"]["position"].asInteger();
    if (position == 0)
      videodatabase.ClearBookMarksOfFile(details.m_strFileNameAndPath, CBookmark::RESUME);
    else
    {
      CBookmark bookmark;
      int total = (int)parameterObject["resume"]["total"].asInteger();
      if (total <= 0 && !videodatabase.GetResumeBookMark(details.m_strFileNameAndPath, bookmark))
        bookmark.totalTimeInSeconds = details.m_streamDetails.GetVideoDuration();
      else
        bookmark.totalTimeInSeconds = total;

      bookmark.timeInSeconds = position;
      videodatabase.AddBookMarkToFile(details.m_strFileNameAndPath, bookmark, CBookmark::RESUME);
    }
  }
}

void CVideoLibrary::UpdateVideoTagField(const CVariant& parameterObject, const std::string& fieldName, std::vector<std::string>& fieldValue, std::set<std::string>& updatedDetails)
{
  if (ParameterNotNull(parameterObject, fieldName))
  {
    CopyStringArray(parameterObject[fieldName], fieldValue);
    updatedDetails.insert(fieldName);
  }
}

void CVideoLibrary::UpdateVideoTag(const CVariant &parameterObject, CVideoInfoTag& details, std::map<std::string, std::string> &artwork, std::set<std::string> &removedArtwork, std::set<std::string> &updatedDetails)
{
  if (ParameterNotNull(parameterObject, "title"))
    details.SetTitle(parameterObject["title"].asString());
  if (ParameterNotNull(parameterObject, "playcount"))
    details.m_playCount = (int)parameterObject["playcount"].asInteger();
  if (ParameterNotNull(parameterObject, "runtime"))
    details.m_duration = (int)parameterObject["runtime"].asInteger();

  std::vector<std::string> director(details.m_director);
  UpdateVideoTagField(parameterObject, "director", director, updatedDetails);
  details.SetDirector(director);

  std::vector<std::string> studio(details.m_studio);
  UpdateVideoTagField(parameterObject, "studio", studio, updatedDetails);
  details.SetStudio(studio);

  if (ParameterNotNull(parameterObject, "year"))
    details.m_iYear = (int)parameterObject["year"].asInteger();
  if (ParameterNotNull(parameterObject, "plot"))
    details.SetPlot(parameterObject["plot"].asString());
  if (ParameterNotNull(parameterObject, "album"))
    details.SetAlbum(parameterObject["album"].asString());

  std::vector<std::string> artist(details.m_artist);
  UpdateVideoTagField(parameterObject, "artist", artist, updatedDetails);
  details.SetArtist(artist);

  std::vector<std::string> genre(details.m_genre);
  UpdateVideoTagField(parameterObject, "genre", genre, updatedDetails);
  details.SetGenre(genre);

  if (ParameterNotNull(parameterObject, "track"))
    details.m_iTrack = (int)parameterObject["track"].asInteger();
  if (ParameterNotNull(parameterObject, "rating"))
    details.m_fRating = parameterObject["rating"].asFloat();
  if (ParameterNotNull(parameterObject, "userrating"))
    details.m_iUserRating = parameterObject["userrating"].asInteger();
  if (ParameterNotNull(parameterObject, "mpaa"))
    details.SetMPAARating(parameterObject["mpaa"].asString());
  if (ParameterNotNull(parameterObject, "imdbnumber"))
    details.SetIMDBNumber(parameterObject["imdbnumber"].asString());
  if (ParameterNotNull(parameterObject, "premiered"))
    SetFromDBDate(parameterObject["premiered"], details.m_premiered);
  if (ParameterNotNull(parameterObject, "votes"))
    details.SetVotes(parameterObject["votes"].asString());
  if (ParameterNotNull(parameterObject, "lastplayed"))
    SetFromDBDateTime(parameterObject["lastplayed"], details.m_lastPlayed);
  if (ParameterNotNull(parameterObject, "firstaired"))
    SetFromDBDate(parameterObject["firstaired"], details.m_firstAired);
  if (ParameterNotNull(parameterObject, "productioncode"))
    details.SetProductionCode(parameterObject["productioncode"].asString());
  if (ParameterNotNull(parameterObject, "season"))
    details.m_iSeason = (int)parameterObject["season"].asInteger();
  if (ParameterNotNull(parameterObject, "episode"))
    details.m_iEpisode = (int)parameterObject["episode"].asInteger();
  if (ParameterNotNull(parameterObject, "originaltitle"))
    details.SetOriginalTitle(parameterObject["originaltitle"].asString());
  if (ParameterNotNull(parameterObject, "trailer"))
    details.SetTrailer(parameterObject["trailer"].asString());
  if (ParameterNotNull(parameterObject, "tagline"))
    details.SetTagLine(parameterObject["tagline"].asString());
  if (ParameterNotNull(parameterObject, "plotoutline"))
    details.SetPlotOutline(parameterObject["plotoutline"].asString());

  std::vector<std::string> credits(details.m_writingCredits);
  UpdateVideoTagField(parameterObject, "writer", credits, updatedDetails);
  details.SetWritingCredits(credits);

  std::vector<std::string> country(details.m_country);
  UpdateVideoTagField(parameterObject, "country", country, updatedDetails);
  details.SetCountry(country);

  if (ParameterNotNull(parameterObject, "top250"))
    details.m_iTop250 = (int)parameterObject["top250"].asInteger();
  if (ParameterNotNull(parameterObject, "sorttitle"))
    details.SetSortTitle(parameterObject["sorttitle"].asString());
  if (ParameterNotNull(parameterObject, "episodeguide"))
    details.SetEpisodeGuide(parameterObject["episodeguide"].asString());
  if (ParameterNotNull(parameterObject, "set"))
  {
    details.SetSet(parameterObject["set"].asString());
    updatedDetails.insert("set");
  }

  std::vector<std::string> showLink(details.m_showLink);
  UpdateVideoTagField(parameterObject, "showlink", showLink, updatedDetails);
  details.SetShowLink(showLink);

  std::vector<std::string> tags(details.m_tags);
  UpdateVideoTagField(parameterObject, "tag", tags, updatedDetails);
  details.SetTags(tags);

  if (ParameterNotNull(parameterObject, "thumbnail"))
  {
    std::string value = parameterObject["thumbnail"].asString();
    artwork["thumb"] = StringUtils::Trim(value);
    updatedDetails.insert("art.altered");
  }
  if (ParameterNotNull(parameterObject, "fanart"))
  {
    std::string value = parameterObject["fanart"].asString();
    artwork["fanart"] = StringUtils::Trim(value);
    updatedDetails.insert("art.altered");
  }

  if (ParameterNotNull(parameterObject, "art"))
  {
    CVariant art = parameterObject["art"];
    for (CVariant::const_iterator_map artIt = art.begin_map(); artIt != art.end_map(); artIt++)
    {
      if (artIt->second.isString() && !artIt->second.asString().empty())
      {
        artwork[artIt->first] = CTextureUtils::UnwrapImageURL(artIt->second.asString());
        updatedDetails.insert("art.altered");
      }
      else if (artIt->second.isNull())
      {
        artwork.erase(artIt->first);
        removedArtwork.insert(artIt->first);
      }
    }
  }
}
