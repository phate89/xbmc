#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import xbmcplugin
import xbmcgui
import xbmc
import xbmcaddon
import sys
import urllib
import urlparse
import requests
import re
import tmdbsimple as tmdb
from datetime import datetime, timedelta

tmdb.API_KEY = '6889f6089877fd092454d00edb44a84d'
ADDON = xbmcaddon.Addon()
ID = ADDON.getAddonInfo('id')
HANDLE = int(sys.argv[1])

# log function
def log(msg):
    xbmc.log(msg='{addon}: {msg}'.format(addon=ID, msg=msg), level=xbmc.LOGDEBUG)

# get addon url params
def get_params():
    if not sys.argv[2]:
        return {}
    return dict(urlparse.parse_qsl(sys.argv[2].lstrip('?')))

def get_date_numeric(dt):
    return (dt-datetime(1970,1,1)).total_seconds()
    
# get urls and update them if they don't exist or they're too old
def load_base_urls():
    tmdburls={}
    tmdburls['original'] = ADDON.getSetting('originalUrl')
    tmdburls['preview'] = ADDON.getSetting('previewUrl')
    lastUpdated = ADDON.getSetting('lastUpdated')
    if tmdburls['original'] == "" or tmdburls['preview'] == "" or not lastUpdated or float(lastUpdated) < get_date_numeric(datetime.now() - timedelta(days=30)):
        conf = tmdb.Configuration().info()
        if conf:
            tmdburls['original'] = conf['images']['base_url'] + 'original'
            tmdburls['preview'] = conf['images']['base_url'] + 'w780'
            ADDON.setSetting('originalUrl', tmdburls['original'])
            ADDON.setSetting('previewUrl', tmdburls['preview'])
    return tmdburls

# get the specific setting
def getSetting(setting):
    return ADDON.getSetting(setting).strip()

# set the specific setting
def setSetting(setting,value):
    ADDON.setSetting(id=setting, value=str(value))

def search_movies(title, year=None):
    search = tmdb.Search()
    response = search.movie(query=title, year=year, language=ADDON.getSetting('language'))
    return search.results

# find the youtube trailer in the list
def get_trailer(trailers):
    if 'youtube' in trailers:
        for trailer in trailers['youtube']:
            return 'plugin://plugin.video.youtube/?action=play_video&videoid='+trailer['source']
    return ''

# helper function to get the names from a list of dictionaries
def get_names(names):
    nms = []
    if names:
        for name in names:
            nms.append(name['name'])
    return nms
    
# get the cast list
def get_cast_members(casts, type, department, jobs):
    writers = []
    if type in casts:
        for cast in casts[type]:
            if cast['department'] == department and cast['job'] in jobs:
                writers.append(cast['name'])
    return writers

# add the found movies to the list
def add_movies(title,year=None):
    log('Find movie with title {title} from year {year}'.format(title=title, year=year))
    for movie in search_movies(title,year):
        if movie['poster_path']:
            liz=xbmcgui.ListItem(movie['title'], thumbnailImage=tmdburls['preview']+movie['poster_path'], offscreen=True)
        else:
            liz=xbmcgui.ListItem(movie['title'], offscreen=True)
        #liz.setProperty('relevance', '0.5')
        xbmcplugin.addDirectoryItem(handle=HANDLE, url=str(movie['id']), listitem=liz, isFolder=True)

# get the movie info via tmdbsimple
def get_tmdb_movie(mid, parseAll = True):
    movie = tmdb.Movies(mid)
    try:
        if parseAll:
            movie.info(language=ADDON.getSetting('language'), append_to_response='trailers,releases,casts')
        movie.images()
        return movie
    except:
        return None

# get the movie info via imdb
def get_imdb_info(id):
    top250 = 0
    votes = 0
    rating = 0
    r=requests.get('http://www.imdb.com/title/'+id+'/ratings')
    if r.status_code == 200:
        res =re.search(r'<p>(\d+).*?title\?user_rating\=.*?\">(.*?)</a>', r.text)
        if (res):
            votes = int(res.group(1).replace(',',''))
            rating = float(res.group(2))
        res = re.search(r'\#(\d+).*?"/chart/top',r.text)
        if (res):
            top250 = res.group(1)
    return {'votes':votes,'rating':rating,'top250':top250}

# get the artworks from the movie info
def get_artworks(liz, movie):
    if ADDON.getSetting('fanart') == 'true':
        fanarts=[]
        for img in movie.backdrops:
            fanarts.append({'image': tmdburls['original']+img['file_path'], 
                            'preview': tmdburls['preview']+img['file_path']})
        liz.setAvailableFanart(fanarts)
    language = ADDON.getSetting('language').lower()
    for img in movie.posters:
        if img['iso_639_1'] == language:
            liz.addAvailableThumb(tmdburls['original']+img['file_path'], "poster")


# get the details of the found movie
def get_details(mid):
        movie = get_tmdb_movie(mid)
        if movie:
            if ADDON.getSetting('keeporiginaltitle') == 'true':
                title = movie.original_title;
            else:
                title = movie.title;
            liz=xbmcgui.ListItem(title, offscreen=True)
            mpaa = ''
            releasedate = ''
            if 'countries' in movie.releases:
                certprefix = ADDON.getSetting('certprefix')
                certcountry = ADDON.getSetting('tmdbcertcountry').upper()
                for c in movie.releases['countries']:
                    if c['iso_3166_1'] == certcountry:
                        mpaa = certprefix + c['certification']
                        releasedate = c['release_date']
            set = ''
            if movie.belongs_to_collection and 'name' in movie.belongs_to_collection:
                set = movie.belongs_to_collection['name']
            imdb_info = get_imdb_info(movie.imdb_id)
            liz.setInfo('video',
                        {'title': title,
                         'originaltitle': movie.original_title,
                         'top250': imdb_info['top250'],
                         'plot': movie.overview,
                         'tagline': movie.tagline,
                         'duration': movie.runtime * 60,
                         'mpaa': mpaa,
                         'trailer': get_trailer(movie.trailers) if ADDON.getSetting('trailer') == 'true' else "",
                         'genre': get_names(movie.genres),
                         'country': get_names(movie.production_countries),
                         'credits': get_cast_members(movie.casts, 'crew', 'Writing', ['Screenplay', 'Writer' ,'Author']),
                         'director': get_cast_members(movie.casts, 'crew', 'Directing', ['Director']),
                         'set': set,
                         'studio': get_names(movie.production_companies),
                         'premiered': releasedate
                        })

            if imdb_info['votes']>0:
                imdbDef = ADDON.getSetting('RatingS') == 'IMDb'
                liz.setRating("imdb", imdb_info['rating'], imdb_info['votes'], imdbDef )
                liz.setRating("tmdb", movie.vote_average, movie.vote_count, not imdbDef)
            else:
                liz.setRating("tmdb", movie.vote_average, movie.vote_count, True)
            liz.setUniqueIDs({'tmdb' : movie.id, 'imdb': movie.imdb_id}, 'tmdb')
            cast = []
            if 'cast' in movie.casts:
                for actor in movie.casts['cast']:
                    cast.append({'name': actor['name'], 
                                 'role': actor['character'], 
                                 'thumbnail': tmdburls['original']+actor['profile_path'] if actor['profile_path'] else "", 
                                 'order': actor['order']}) 
            liz.setCast(cast)
            get_artworks(liz, movie)
            liz.setProperty('video.date_added', '2016-01-01')
            xbmcplugin.setResolvedUrl(handle=HANDLE, succeeded=True, listitem=liz)

# add the found artworks to the list
def add_artworks(mid):
        movie = get_tmdb_movie(mid)
        if movie:
            liz=xbmcgui.ListItem(movie.title, offscreen=True)
            get_artworks(liz, movie)
            xbmcplugin.setResolvedUrl(handle=HANDLE, succeeded=True, listitem=liz)

def run():
    params=get_params()
    if 'action' in params:
        global tmdburls
        tmdburls=load_base_urls()
        action=urllib.unquote_plus(params["action"])
        if action == 'find' and 'title' in params:
            add_movies(urllib.unquote_plus(params["title"]), params.get("year", None))
        elif action == 'getdetails' and 'url' in params:
            get_details(urllib.unquote_plus(params["url"]))
        elif action == 'getartwork' and 'id' in params:
            add_artworks(urllib.unquote_plus(params["id"]))
    xbmcplugin.endOfDirectory(HANDLE)
