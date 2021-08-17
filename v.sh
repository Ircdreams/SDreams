grep -e ' \* \$Id *' src/*.c > tmp.gen
version=`awk '{split($5, tmp, "."); version += tmp[1]*1000 + tmp[2]} END {print version}' tmp.gen`

#grep -e '^CVSINFO(' include/*.h > tmp.gen
#versionh=`awk '{split($4, tmp, "."); version += tmp[1]*1000 + tmp[2]} END {print version}' tmp.gen`

rm tmp.gen
#version=`expr $versionh + $versionc`

if [ -f "include/version.h" ]; then
	currentv=`sed -n 's/^\#define CVSVERSION \"\(.*\)\"/\1/p' < include/version.h`
else
	currentv=0
fi

if [ "$version" != "$currentv" ]; then

/bin/cat > include/version.h <<!SUB!THIS!

/* include/version.h - fichier de version généré
 *
 * Copyright (C) 2002-2005 David Cortier  <Cesar@ircube.org>
 *                         Romain Bignon  <Progs@ir3.org>
 *                         Benjamin Beret <kouak@kouak.org>
 *
 * site web: http://sf.net/projects/scoderz/
 *
 * Services pour serveur IRC. Supporté sur IrcProgs et IRCoderz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define CVSVERSION "$version"

!SUB!THIS!

fi
