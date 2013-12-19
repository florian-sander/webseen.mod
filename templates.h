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

#define TEMPLATE_ENGINE_VERSION "1.0.0"

static void init_templates();
static int expmem_templates();
static void unload_templates();

// template header struct. Stores the name and a pointer to the content
// of an template.
struct templates {
  struct templates *next;
  char *name;
  struct tpl_content *contents;
};

// template content struct. Stores the content (html and command pointers)
// of an template
struct tpl_content {
  struct tpl_content *next;
  char *html;
  void (*command) (int, struct tpl_content *);
  int what;
  char *charpar1;
  float floatpar1;
  float floatpar2;
  int intpar1;
  struct tpl_content *subcontent;
};

// template language struct
// contains the language name and a pointer to the template itself
struct template_language {
  struct template_language *next;
  char *language;
  struct templates *tpl;
};

// template skin struct
// contains the name of the skin and a pointer to the language-list
struct template_skins {
  char *skin;
  struct template_skins *next;
  struct template_language *langs;
};

// "ger" == "Deutsch",
// "default" == "Google"
// etc...
struct template_descriptions {
  struct template_descriptions *next;
  char *name;
  char *desc;
};

static int expmem_tpl_content(struct tpl_content *);
static void free_template_content(struct tpl_content *);
static void free_skins(struct template_skins *);
static void free_language(struct template_language *);
static void free_template(struct templates *);
static void free_descriptions(struct template_descriptions *);
static void template_destroy(char *, char *, char *);
static void add_description(char *, char *, char *);
static int loadtemplate(char *, char *, char *, char *);
static int template_parse_content(struct templates *, struct tpl_content *, char *);
static struct templates *template_find(char *, char *, char *);
static struct templates *template_create(char *, char *, char *);
static void template_add_html(struct tpl_content *, char *);
static void template_add_command(struct tpl_content *, char *);
static void template_send(int, char *);
static void template_send_contents(int, struct tpl_content *);
static struct tpl_content *template_create_content(struct templates *);
static struct tpl_content *template_create_subcontent(struct tpl_content *);
static void template_add_cmd_template(struct tpl_content *, char *);
static void template_send_template(int, struct tpl_content *);
static void template_send_module_version(int, struct tpl_content *);
static int template_add_custom_command(struct tpl_content *, char *);
static void template_add_cmd_langlist(struct tpl_content *, char *);
static void template_add_cmd_skinlist(struct tpl_content *, char *);
static void template_send_langlist(int, struct tpl_content *);
static void template_send_skinlist(int, struct tpl_content *);
static void template_add_cmd_ifactivelang(struct tpl_content *, char *);
static void template_send_ifactivelang(int, struct tpl_content *);
static void template_add_cmd_ifactiveskin(struct tpl_content *, char *);
static void template_send_ifactiveskin(int, struct tpl_content *);
static void template_send_name(int, struct tpl_content *);
static void template_send_desc(int, struct tpl_content *);
static void template_send_server_version(int, struct tpl_content *);
static void template_send_server_port(int, struct tpl_content *);
static void template_send_requested_url(int, struct tpl_content *);
