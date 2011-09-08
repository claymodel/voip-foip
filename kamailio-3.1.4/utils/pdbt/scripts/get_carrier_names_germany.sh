#!/bin/bash

# Copyright (C) 2009 1&1 Internet AG
#
# This file is part of sip-router, a free SIP server.
#
# sip-router is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version
#
# sip-router is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License 
# along with this program; if not, write to the Free Software 
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Small helper script for germany:
# Download the directory of german carrier names and the respective IDs from
# the 'Bundesnetzagentur' and convert this into the format which the pdbt tool
# understands.

url="http://www.bundesnetzagentur.de/cln_1912/DE/Sachgebiete/Telekommunikation/RegulierungTelekommunikation/Nummernverwaltung/TechnischeNummern/Portierungskennung/VerzeichnisPortKenn_Basepage.html"

# fix LOCALE problem during filtering 
export LANG="C"

wget -O - "$url" | recode latin1..utf8 | sed 's/^*.Verzeichnis der Portierungskennungen//' | awk '/<tbody>/, /<\/tbody>/' | tr -d '\r' | tr '\n' '@' | sed 's/<\/table>.*$//' | sed 's/<\/tbody>.*$//'

