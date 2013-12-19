/*
 * Copyright (C) 2001  Florian Sander
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* init_global_vars()
 * inits the global variables to make sure that no weird
 * bugs appear due to unitialized vars, or vars with content
 * from the last request
 */
static void init_global_vars()
{
  glob_results = glob_result = NULL;
  sglobseendat = NULL;
  glob_fuzzy = fuzzy_search;
  glob_seenerror = 0;
  glob_searchtime = 0.0;
}

/* send_webseen():
 * take the URL string, split the parameters off,
 * calculate seen-results if necessary, and finally
 * send a template to the client
 */
static void process_get_request(int idx)
{
  char *url = NULL;
  char *tpl;
  char *fuzzy;
  double presearch, aftersearch;
  struct timeval t;

  Context;
  // init all global vars
  init_global_vars();
  // init the global seenlang settings
  setsglobs(NULL, "www-user");
  //
  url = http_connection(idx)->get;
  if (!url) {
    debug1("%s: no request. Dropping connection.", dcc[idx].host);
    return;
  }
  // now decode the URL
  url = decode_url(url);
  // time to get all settings from cookies and parameters
  // first try to get the setting from a parameter, if this fails,
  // try from cookies and if there's also no matching cookie, then
  // just point to a static pointer
  if (!(template_lang = get_param_value(idx, "lang")))
    if (!(template_lang = get_cookie_value(idx, "lang")))
      template_lang = default_lang;
  if (!(template_skin = get_param_value(idx, "skin")))
    if (!(template_skin = get_cookie_value(idx, "skin")))
      template_skin = default_skin;
  // query is either defined as parameter or ""
  if (!(glob_query = get_param_value(idx, "query")))
    glob_query = "";
  // the following settings can only be specified as parameters
  // (cookie or default would be kinda stupid)
  tpl = get_param_value(idx, "template");
  fuzzy = get_param_value(idx, "fuzzy");
  if (fuzzy)
    glob_fuzzy = atoi(fuzzy);
  else
    glob_fuzzy = fuzzy_search;
  // if a valid language was specified, use this one as seen-lang.
  // if not, use the default (NULL)
  if (valid_seenlang(template_lang)) {
    debug1("activating lang: %s", template_lang);
    setseenlang(template_lang);
  } else {
    debug0("activating default lang :/");
    setseenlang(NULL);
  }
  // now it's time to choose what to do
  if (!strcmp(url, "/")) {
    // user accessed the server root? ok, send the root template...
    send_http_header(idx, 200);
    template_send(idx, "root");
  } else if (!strcmp(url, "/seen")) {
    // oh, looks like a seen-query... let's process it.
    gettimeofday(&t, NULL);
    presearch = (float) t.tv_sec + (((float) t.tv_usec) / 1000000);
    glob_results = findseens(glob_query, &glob_seenerror, glob_fuzzy);
    gettimeofday(&t, NULL);
    aftersearch = (float) t.tv_sec + (((float) t.tv_usec) / 1000000);
    glob_searchtime = aftersearch - presearch;
    if (!glob_results) {
      // glob_results is NULL, so either there was really no result, or
      // an error occured
      if (glob_seenerror == WS_NORESULT) {
        send_http_header(idx, 200);
        template_send(idx, "noresult");
      } else {
        send_http_header(idx, 200);
        template_send(idx, "error");
      }
    } else {
      // ok, we got our results, so lets output them
      send_http_header(idx, 200);
      template_send(idx, "results");
    }
  } else {
    send_http_header(idx, 404);
    template_send(idx, "404");
  }
  if (glob_results)
    free_seenresults();
}