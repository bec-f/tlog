m4_dnl
m4_dnl Tlog-rec configuration schema
m4_dnl
m4_dnl See conf_schema.m4 for macro invocations expected here.
m4_dnl
m4_dnl Copyright (C) 2016 Red Hat
m4_dnl
m4_dnl This file is part of tlog.
m4_dnl
m4_dnl Tlog is free software; you can redistribute it and/or modify
m4_dnl it under the terms of the GNU General Public License as published by
m4_dnl the Free Software Foundation; either version 2 of the License, or
m4_dnl (at your option) any later version.
m4_dnl
m4_dnl Tlog is distributed in the hope that it will be useful,
m4_dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
m4_dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
m4_dnl GNU General Public License for more details.
m4_dnl
m4_dnl You should have received a copy of the GNU General Public License
m4_dnl along with tlog; if not, write to the Free Software
m4_dnl Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
m4_dnl
m4_include(`conf_schema.m4')m4_dnl
m4_dnl
m4_pushdef(`_M4_PARAM', `M4_PARAM($@)')m4_dnl
m4_include(`rec_common_conf_schema.m4')m4_dnl
m4_popdef(`_M4_PARAM')m4_dnl
m4_dnl
M4_PARAM(`', `writer', `file-',
         `M4_TYPE_CHOICE(`file', `syslog', `file')', true,
         `w', `=STRING', `Use STRING log writer (syslog/file, default file)',
         `M4_LINES(`The type of "log writer" to use for logging. The writer needs',
                   `to be configured using its dedicated parameters.')')m4_dnl
m4_dnl
