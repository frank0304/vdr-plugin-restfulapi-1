#include "webapp.h"

void WebappResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply) {

  QueryHandler::addHeader(reply);

  if ( request.method() == "OPTIONS" ) {
      reply.addHeader("Allow", "GET");
      reply.httpReturn(200, "OK");
      return;
  }
  if ( request.method() != "GET") {
     reply.httpReturn(403, "To retrieve files use the GET method!");
     return;
  }

  double timediff = -1;
  string url = request.url();
  string base = "/webapp/";

  if ( (int)url.find(base) == 0 ) {

      esyslog("restfulapi Webapp: file request url %s", request.url().c_str());

      QueryHandler::addHeader(reply);
      string fileName = getFileName(base, url);
      string file = getFile(fileName);

      if (!FileExtension::get()->exists(file)) {
	  esyslog("restfulapi Webapp: file does not exist");
	  reply.httpReturn(404, "File not found");
	  return;
      }

      if (request.hasHeader("If-Modified-Since")) {
	  timediff = difftime(FileExtension::get()->getModifiedTime(file), FileExtension::get()->getModifiedSinceTime(request));
      }
      if (timediff > 0.0 || timediff < 0.0) {
	  streamResponse(fileName, out, file, reply);
      } else {
	  esyslog("restfulapi Webapp: file not modified, returning 304");
	  reply.httpReturn(304, "Not-Modified");
      }
  }
}

/**
 * retrieve filename width path
 * @param string fileName
 */
string WebappResponder::getFile(std::string fileName) {

  string webappPath = Settings::get()->WebappDirectory();
  return webappPath + (string)"/" + fileName;
};

/**
 * retrieve filename
 * @param string base url
 * @param string url
 */
string WebappResponder::getFileName(string base, string url) {

  const char *empty = "";
  string file = url.replace(0, base.length(), "");

  if (strcmp(file.c_str(), empty) == 0) {
      file = "index.html";
  }
  return file;
};

/**
 * determine contenttype
 * @param string fileName
 */
const char *WebappResponder::getContentType(string fileName) {

  const char *type = fileName.substr(fileName.find_last_of(".")+1).c_str();
  const char *contentType = "";
  const char *extHtml = "html";
  const char *extJs = "js";
  const char *extCss = "css";
  const char *extJpg = "jpg";
  const char *extJpeg = "jpeg";
  const char *extGif = "gif";
  const char *extPng = "png";
  const char *extIco = "ico";
  const char *extAppCacheManifest = "appcache";
  esyslog("restfulapi Webapp: file type %s", type);

  if ( strcmp(type, extHtml) == 0 ) {
      contentType = "text/html";
  } else if ( strcmp(type, extJs) == 0 ) {
      contentType = "application/javascript";
  } else if ( strcmp(type, extCss) == 0 ) {
      contentType = "text/css";
  } else if ( strcmp(type, extJpg) == 0 || strcmp(type, extJpeg) == 0 || strcmp(type, extGif) == 0 || strcmp(type, extPng) == 0 ) {
      contentType = ("image/" + (string)type).c_str();
  } else if ( strcmp(type, extIco) == 0 ) {
      contentType = "image/x-icon";
  } else if ( strcmp(type, extAppCacheManifest) == 0 ) {
      contentType = "text/cache-manifest";
  }

  return contentType;
};

void WebappResponder::streamResponse(string fileName, ostream& out, string file, cxxtools::http::Reply& reply) {

  const char *empty = "";
  const char * contentType = getContentType(fileName);

  StreamExtension se(&out);
  if ( strcmp(contentType, empty) != 0 && se.writeBinary(file) ) {
      esyslog("restfulapi Webapp: successfully piped file");
      FileExtension::get()->addModifiedHeader(file, reply);
      reply.addHeader("Content-Type", contentType);
  } else {
      esyslog("restfulapi Webapp: error piping file");
      reply.httpReturn(404, "File not found");
  }
};