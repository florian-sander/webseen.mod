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
/* templates.c
 *
 * template engine for use with mini_httpd.c
 *
 * supports multiple languages as skins
 *
 * Usage:
 *
 * add init_templates() to module_start()
 * add unload_templates() to module_close()
 * add expmem_templates() to module_expmem()
 *
 * Call loadtemplate(skin, lang, name, file)
 */

static struct template_skins *template_list;
static struct template_descriptions *skin_descriptions, *lang_descriptions, *glob_desc;

static float glob_r, glob_g, glob_b, glob_rstep, glob_gstep, glob_bstep;

static char *template_skin, *template_lang;
static char *default_skin, *default_lang;

/* init_templates()
 * initializes some global variables
 */
static void init_templates()
{
  template_skin = NULL;
  template_lang = NULL;
  template_list = NULL;
  skin_descriptions = NULL;
  lang_descriptions = NULL;
  glob_desc = NULL;
  default_skin = nmalloc(strlen("default") + 1);
  strcpy(default_skin, "default");
  default_lang = nmalloc(strlen("eng") + 1);
  strcpy(default_lang, "eng");
}

/* unload_templates()
 * removes every template-related stuff from memory
 */
static void unload_templates()
{
  free_skins(template_list);
  template_list = NULL;
  free_descriptions(skin_descriptions);
  free_descriptions(lang_descriptions);
  Assert(default_skin);
  Assert(default_lang);
  nfree(default_skin);
  nfree(default_lang);
}

/* expmem_templates():
 * returns the memory usage of the template-system
 */
static int expmem_templates()
{
  int size = 0;
  struct templates *tpl;
  struct template_skins *skin;
  struct template_language *lang;
  struct template_descriptions *desc;

  Context;
  for (skin = template_list; skin; skin = skin->next) {
    size += sizeof(struct template_skins);
    size += strlen(skin->skin) + 1;
    for (lang = skin->langs; lang; lang = lang->next) {
      size += sizeof(struct template_language);
      size += strlen(lang->language) + 1;
      for (tpl = lang->tpl; tpl; tpl = tpl->next) {
        size += sizeof(struct templates);
        size += strlen(tpl->name) + 1;
        size += expmem_tpl_content(tpl->contents);
      }
    }
  }
  for (desc = skin_descriptions; desc; desc = desc->next) {
    size += sizeof(struct template_descriptions);
    size += strlen(desc->name) + 1;
    size += strlen(desc->desc) + 1;
  }
  for (desc = lang_descriptions; desc; desc = desc->next) {
    size += sizeof(struct template_descriptions);
    size += strlen(desc->name) + 1;
    size += strlen(desc->desc) + 1;
  }
  Assert(default_skin);
  Assert(default_lang);
  size += strlen(default_skin) + 1;
  size += strlen(default_lang) + 1;
  Context;
  return size;
}

/* expmem_tpl_content():
 * returns the used memory of the content of a template
 */
static int expmem_tpl_content(struct tpl_content *tpc)
{
  int size = 0;

  for (;tpc; tpc = tpc->next) {
    size += sizeof(struct tpl_content);
    if (tpc->html)
      size += strlen(tpc->html) + 1;
    if (tpc->charpar1)
      size += strlen(tpc->charpar1) + 1;
    size += expmem_tpl_content(tpc->subcontent);
  }
  return size;
}

static void free_descriptions(struct template_descriptions *desc)
{
  struct template_descriptions *next;

  while (desc) {
    next = desc->next;
    nfree(desc->name);
    nfree(desc->desc);
    nfree(desc);
    desc = next;
  }
}

static void free_skins(struct template_skins *skin)
{
  struct template_skins *next;

  while (skin) {
    next = skin->next;
    free_language(skin->langs);
    nfree(skin->skin);
    nfree(skin);
    skin = next;
  }
}

static void free_language(struct template_language *lang)
{
  struct template_language *next;

  while (lang) {
    next = lang->next;
    free_template(lang->tpl);
    nfree(lang->language);
    nfree(lang);
    lang = next;
  }
}

static void free_template(struct templates *tpl)
{
  struct templates *next;

  while (tpl) {
    next = tpl->next;
    free_template_content(tpl->contents);
    nfree(tpl->name);
    nfree(tpl);
    tpl = next;
  }
}

/* free_template_content():
 * frees the content of a template
 */
static void free_template_content(struct tpl_content *tpc)
{
  struct tpl_content *next;

  Context;
  while (tpc) {
    if (tpc->html)
      nfree(tpc->html);
    if (tpc->charpar1)
      nfree(tpc->charpar1);
    free_template_content(tpc->subcontent);
    next = tpc->next;
    nfree(tpc);
    tpc = next;
  }
  Context;
}

/* template_destroy():
 * deletes a single template if it exists
 */
static void template_destroy(char *skin, char *lang, char *name)
{
  struct templates *tpl, *ltpl;
  struct template_skins *s;
  struct template_language *l;

  Context;
  Assert(lang);
  Assert(skin);
  for (s = template_list; s; s = s->next) {
    if (!strcasecmp(s->skin, skin)) {
      for (l = s->langs; l; l = l->next) {
        if (!strcasecmp(l->language, lang)) {
          tpl = l->tpl;
          ltpl = NULL;
          while (tpl) {
            if (!strcasecmp(tpl->name, name)) {
              debug1("Destroying template \"%s\".", name);
              free_template_content(tpl->contents);
              if (ltpl)
                ltpl->next = tpl->next;
              else
                l->tpl = tpl->next;
              nfree(tpl->name);
              nfree(tpl);
              return;
            }
            ltpl = tpl;
            tpl = tpl->next;
          }
        }
      }
    }
  }
}

/* add_description()
 * adds a language or skin to the list of descriptions
 * (only used for showing a list of options)
 */
static void add_description(char *type, char *name, char *desc)
{
  struct template_descriptions *ndesc, *list;

  if (!strcasecmp(type, "lang"))
    list = lang_descriptions;
  else
    list = skin_descriptions;
  for (; list; list = list->next)
    if (!strcasecmp(list->name, name))
      return;
  ndesc = nmalloc(sizeof(struct template_descriptions));
  ndesc->name = nmalloc(strlen(name) + 1);
  strcpy(ndesc->name, name);
  ndesc->desc = nmalloc(strlen(desc) + 1);
  strcpy(ndesc->desc, desc);
  ndesc->next = NULL;
  if (!strcasecmp(type, "lang")) {
    ndesc->next = lang_descriptions;
    lang_descriptions = ndesc;
  } else {
    ndesc->next = skin_descriptions;
    skin_descriptions = ndesc;
  }
  debug2("Added description \"%s\" for \"%s\".", desc, name);
}

#define TEMPLATE_LINE_LENGTH 1024
/* template_load():
 * loads a template into memory
 */
static int loadtemplate(char *skin, char *lang, char *name, char *file)
{
  FILE *f;
  char buf[TEMPLATE_LINE_LENGTH + 1], *content;
  int ret;
  struct templates *tpl;

  Context;
  // at first, load the whole file into a buffer
  f = fopen(file, "r");
  if (f == NULL) {
    putlog(LOG_MISC, "*", "Couldn't open template \"%s\" (%s)!", name, file);
    return 0;
  }
  content = nmalloc(1);
  content[0] = 0;
  while (!feof(f)) {
    if (fgets(buf, TEMPLATE_LINE_LENGTH, f)) {
      buf[TEMPLATE_LINE_LENGTH] = 0;
      content = nrealloc(content, strlen(content) + strlen(buf) + 1);
      strcat(content, buf);
    }
  }
  fclose(f);
  // then create a new template
  tpl = template_create(skin, lang, name);
  // now process the content and store the return value
  ret = template_parse_content(tpl, NULL, content);
  putlog(LOG_MISC, "*", "Template \"%s\" (%s) loaded.", name, file);
  nfree(content);
  return ret;
}

/* template_parse_content():
 * preprocesses the content and fills a template with html-code and commands
 */
static int template_parse_content(struct templates *h_tpl, struct tpl_content *h_tpc, char *content)
{
  struct tpl_content *tpc;
  char *s, *cmdstart;
  int open_commands;

  Context;
  Assert(content);
  Assert(h_tpl || h_tpc);
  cmdstart = NULL;
  // and now start parsing....
  s = content;
  open_commands = 0;
  while (s[0] && s[1]) {
    // check for start of a command
    if (!strncasecmp(s, "<?", 2) || !strncasecmp(s, "<$", 2)) {
      // increase number of open commands
      open_commands++;
      // if this is not a command within a command then store the start
      // of this command
      // (nested commands get processed by their command handlers later...)
      if (open_commands == 1)
        cmdstart = s;
      // don't forget to increase the pointer, or we'll be stuck in an
      // infinite loop ^_^
      s++;
    } else if (!strncasecmp(s, "?>", 2) || !strncasecmp(s, "$>", 2)) {
      open_commands--;
      if (open_commands < 0) {
        putlog(LOG_MISC, "*",
               "ERROR parsing template! Too many command terminators! (%s)",
               content);
        return 0;
      }
      // if there are no more open commands, then this was a main command
      // which we wanted to process
      if (open_commands == 0) {
        Assert(cmdstart);
        // terminate the string at the start of our command and add it
        // as static html code to the template
        cmdstart[0] = 0;
        if (h_tpl)
          tpc = template_create_content(h_tpl);
        else
          tpc = template_create_subcontent(h_tpc);
        template_add_html(tpc, content);
        // now terminate the end of the command and add it
        s[0] = 0;
        // unless the command is just a comment, add it to the content
        if (!(!strncmp(cmdstart + 2, "--", 2))) {
          if (h_tpl)
            tpc = template_create_content(h_tpl);
          else
            tpc = template_create_subcontent(h_tpc);
          template_add_command(tpc, cmdstart + 2);
        }
        // set the start of content to the first char after the command
        content = s + 2;
        s = content;
        cmdstart = NULL;
      } else {
        s++;
      }
    } else {
      s++;
    }
  }
  // finally add all html code that is still left
  if (h_tpl)
    tpc = template_create_content(h_tpl);
  else
    tpc = template_create_subcontent(h_tpc);
  template_add_html(tpc, content);
  // and last but not least check if there are any open commands left...
  if (open_commands > 0) {
    putlog(LOG_MISC, "*", "ERROR parsing template! %d unterminated command%s!", open_commands, (open_commands > 1) ? "s" : "");
    return 0;
  }
  return 1;
}

/* template_find():
 * returns the pointer to a template (or NULL if the template wasn't found)
 */
static struct templates *template_find(char *skin, char *lang, char *name)
{
  struct templates *tpl;
  struct template_skins *s;
  struct template_language *l;

  Assert(lang);
  Assert(skin);
  // at first, find the current skin
  for (s = template_list; s; s = s->next)
    if (!strcasecmp(s->skin, skin))
      break;
  // if it's not found, output a warning and try using the default skin
  if (!s) {
    debug1("WARNING: skin \"%s\" not found! Trying to use the default....",
           skin);
    for (s = template_list; s; s = s->next)
      if (!strcasecmp(s->skin, default_skin))
        break;
    // if the default skin is also missing, then give up
    if (!s)
      return NULL;
  }
  // now find the chosen language...
  for (l = s->langs; l; l = l->next)
    if (!strcasecmp(l->language, lang))
      break;
  // ... or at least the default one
  if (!l) {
    debug2("WARNING: Language \"%s\" not found in skin \"%s\"! Trying to use "
           "the default...", lang, s->skin);
    for (l = s->langs; l; l = l->next)
      if (!strcasecmp(l->language, default_lang))
        break;
    if (!l)
      return NULL;
  }
  // and finally find and return the template
  for (tpl = l->tpl; tpl; tpl = tpl->next)
    if (!strcasecmp(tpl->name, name))
      return tpl;
  return NULL;
}

/* template_create():
 * creates a new template and returns a pointer to this template.
 */
static struct templates *template_create(char *skin, char *lang, char *name)
{
  struct templates *ntpl;
  struct template_skins *s;
  struct template_language *l;

  Context;
  Assert(lang);
  Assert(skin);
  // if there is already a template with this name, just delete it.
  template_destroy(skin, lang, name);
  // at first, seek the skin to which we want to add our template
  for (s = template_list; s; s = s->next)
    if (!strcasecmp(s->skin, skin))
      break;
  // if there's no entry for this skin, create one
  if (!s) {
    s = nmalloc(sizeof(struct template_skins));
    s->skin = nmalloc(strlen(skin) + 1);
    strcpy(s->skin, skin);
    s->langs = NULL;
    s->next = template_list;
    template_list = s;
  }
  // now find the language
  for (l = s->langs; l; l = l->next)
    if (!strcasecmp(l->language, lang))
      break;
  if (!l) {
    l = nmalloc(sizeof(struct template_language));
    l->language = nmalloc(strlen(lang) + 1);
    strcpy(l->language, lang);
    l->tpl = NULL;
    l->next = s->langs;
    s->langs = l;
  }
  // and finally create the template
  ntpl = nmalloc(sizeof(struct templates));
  ntpl->name = nmalloc(strlen(name) + 1);
  strcpy(ntpl->name, name);
  ntpl->contents = NULL;
  ntpl->next = l->tpl;
  l->tpl = ntpl;
  Context;
  // ok, done. Let's return the pointer.
  return ntpl;
}

/* template_create_content():
 * creates a new initialized content struct
 */
static struct tpl_content *template_create_content(struct templates *tpl)
{
  struct tpl_content *tpc, *ntpc;

  Context;
  tpc = tpl->contents;
  while (tpc && tpc->next)
    tpc = tpc->next;
  ntpc = nmalloc(sizeof(struct tpl_content));
  ntpc->next = NULL;
  ntpc->html = NULL;
  ntpc->command = NULL;
  ntpc->subcontent = NULL;
  ntpc->charpar1 = NULL;
  ntpc->what = 0;
  if (tpc)
    tpc->next = ntpc;
  else
    tpl->contents = ntpc;
  Context;
  return ntpc;
}

/* template_create_subcontent():
 * creates a new initialized content struct within a content struct
 */
static struct tpl_content *template_create_subcontent(struct tpl_content *h_tpc)
{
  struct tpl_content *tpc, *ntpc;

  Context;
  tpc = h_tpc->subcontent;
  while (tpc && tpc->next)
    tpc = tpc->next;
  ntpc = nmalloc(sizeof(struct tpl_content));
  ntpc->next = NULL;
  ntpc->html = NULL;
  ntpc->command = NULL;
  ntpc->subcontent = NULL;
  ntpc->charpar1 = NULL;
  ntpc->what = 0;
  if (tpc)
    tpc->next = ntpc;
  else
    h_tpc->subcontent = ntpc;
  Context;
  return ntpc;
}

/* template_add_html():
 * adds html code to the content of a template
 */
static void template_add_html(struct tpl_content *tpc, char *html)
{
  Context;
  tpc->html = nmalloc(strlen(html) + 1);
  strcpy(tpc->html, html);
  Context;
}

/* template_add_command():
 * adds an command to the content of the template by setting a pointer to
 * a function which will finally do the dynamic output
 */
static void template_add_command(struct tpl_content *tpc, char *command)
{
  Context;
  // strip leading spaces
  while (command[0] == ' ')
    command++;
  if (!strncasecmp(command, "module_version", 14)) {
    tpc->command = template_send_module_version;
  } else if (!strncasecmp(command, "server_version", 14)) {
    tpc->command = template_send_server_version;
  } else if (!strncasecmp(command, "server_port", 11)) {
    tpc->command = template_send_server_port;
  } else if (!strncasecmp(command, "template", 8)) {
    template_add_cmd_template(tpc, command + 8);
  } else if (!strncasecmp(command, "langlist", 8)) {
    template_add_cmd_langlist(tpc, command + 9);
  } else if (!strncasecmp(command, "skinlist", 8)) {
    template_add_cmd_skinlist(tpc, command + 8);
  } else if (!strncasecmp(command, "ifactivelang", 12)) {
    template_add_cmd_ifactivelang(tpc, command + 13);
  } else if (!strncasecmp(command, "ifactiveskin", 12)) {
    template_add_cmd_ifactiveskin(tpc, command + 13);
  } else if (!strncasecmp(command, "name", 4)) {
    tpc->command = template_send_name;
  } else if (!strncasecmp(command, "desc", 4)) {
    tpc->command = template_send_desc;
  } else if (!strncasecmp(command, "requested_url", 13)) {
    tpc->command = template_send_requested_url;
  } else {
    if (!template_add_custom_command(tpc, command)) {
      // if no matching function is found, log an error and write a warning
      // into the template
      putlog(LOG_MISC, "*", "Error parsing template! "
             "Invalid command: %s", command);
      tpc->html = nmalloc(33 + strlen(command) + 1);
      sprintf(tpc->html, "<H1>ERROR: Invalid command: %s</H1>", command);
    }
  }
}

/* template_send():
 * sends an template via template_send_contents()
 */
static void template_send(int idx, char *name)
{
  struct templates *tpl;

  Context;
  debug2("Sending template \"%s\" to idx %d", name, idx);
  tpl = template_find(template_skin, template_lang, name);
  if (!tpl) {
    putlog(LOG_MISC, "*", "Error: template \"%s\"(skin: %s, lang: %s) "
           "not found!", name, template_skin,
           template_lang ? template_lang : "");
    dprintf(idx, "<html><body><H1>ERROR! Template not found: %s(skin: %s, "
            "lang: %s)</H1></body></html>", name, template_skin,
            template_lang ? template_lang : "");
    return;
  }
  template_send_contents(idx, tpl->contents);
  Context;
}

/* template_send_contents():
 * outputs the content of a template by either dumping the static html stuff
 * directly, or by calling the function which outputs the dynamic content.
 */
static void template_send_contents(int idx, struct tpl_content *tpc)
{
  for (;tpc; tpc = tpc->next) {
    if (tpc->html)
      dprintf(idx, "%s", tpc->html);
    else if (tpc->command)
      tpc->command(idx, tpc);
    else
      dprintf(idx, "<H1>ERROR: No content!</H1>");
  }
}

/* template_send_module_version():
 * sends the module version (surprise!)
 */
static void template_send_module_version(int idx, struct tpl_content *tpc)
{
  dprintf(idx, "%s", MODULE_VERSION);
}

/* template_add_cmd_template():
 * initialized <?template?> which sends an template from inside an template
 * (I bet someone will finally create an infinite loop with this...
 *  ...probably I'll be that someone ^_^)
 */
static void template_add_cmd_template(struct tpl_content *h_tpc, char *params)
{
  Context;
  // remove leading spaces
  while (params[0] == ' ')
    params++;
  // put the template name into charpar1
  h_tpc->charpar1 = nmalloc(strlen(params) + 1);
  strcpy(h_tpc->charpar1, params);
  // and finally set the pointer to the sending function
  h_tpc->command = template_send_template;
}

/* template_send_template()
 * sends the template specified in charpar1
 */
static void template_send_template(int idx, struct tpl_content *tpc)
{
  template_send(idx, tpc->charpar1);
}

/* <?langlist ... ?>:
 * outputs a list of available languages by iterating all parameters
 * for each language
 */
static void template_add_cmd_langlist(struct tpl_content *h_tpc, char *params)
{
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_langlist;
}

static void template_send_langlist(int idx, struct tpl_content *h_tpc)
{
  struct template_descriptions *desc;

  for (desc = lang_descriptions; desc; desc = desc->next) {
    glob_desc = desc;
    template_send_contents(idx, h_tpc->subcontent);
  }
}

/* <?skinlist ... ?>:
 * same as langlist, but for skins
 */
static void template_add_cmd_skinlist(struct tpl_content *h_tpc, char *params)
{
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_skinlist;
}

static void template_send_skinlist(int idx, struct tpl_content *h_tpc)
{
  struct template_descriptions *desc;

  for (desc = skin_descriptions; desc; desc = desc->next) {
    glob_desc = desc;
    template_send_contents(idx, h_tpc->subcontent);
  }
}

/* <?ifactivelang ... ?>:
 * used within langlist.
 * outputs the subcontent if the actual language is the active languane
 */
static void template_add_cmd_ifactivelang(struct tpl_content *h_tpc, char *params)
{
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_ifactivelang;
}

static void template_send_ifactivelang(int idx, struct tpl_content *h_tpc)
{
  Assert(template_lang);
  Assert(glob_desc);
  if (!strcasecmp(glob_desc->name, template_lang))
    template_send_contents(idx, h_tpc->subcontent);
}

/* <?ifactiveskin ... ?>:
 * same as ifactivelang, but for skin.
 */
static void template_add_cmd_ifactiveskin(struct tpl_content *h_tpc, char *params)
{
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_ifactiveskin;
}

static void template_send_ifactiveskin(int idx, struct tpl_content *h_tpc)
{
  Assert(template_skin);
  Assert(glob_desc);
  if (!strcasecmp(glob_desc->name, template_skin))
    template_send_contents(idx, h_tpc->subcontent);
}

/* <?name?>
 * outputs the name of the current lang/skin in a langlist/skinlist-cycle
 */
static void template_send_name(int idx, struct tpl_content *tpc)
{
  Assert(glob_desc);
  dprintf(idx, "%s", glob_desc->name);
}

/* <?desc?>
 * outputs the description of the current lang/skin in a lang-/skinlist-cycle
 */
static void template_send_desc(int idx, struct tpl_content *tpc)
{
  Assert(glob_desc);
  dprintf(idx, "%s", glob_desc->desc);
}

/* <?server_version?>
 * outputs the current server version
 */
static void template_send_server_version(int idx, struct tpl_content *tpc)
{
  dprintf(idx, "%s", HTTPD_VERSION);
}

/* <?server_port?>
 * outputs the current server port
 */
static void template_send_server_port(int idx, struct tpl_content *tpc)
{
  if (httpd_dcc_index >= 0)
    dprintf(idx, "%d", dcc[httpd_dcc_index].port);
}

/* <?requested_URL?>
 * outputs the requested url
 */
static void template_send_requested_url(int idx, struct tpl_content *tpc)
{
  Assert(http_connection(idx)->get);
  dprintf(idx, "%s", http_connection(idx)->get);
}