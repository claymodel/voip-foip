<?xml version="1.0" encoding="UTF-8" ?>
<!--
# Copyright 2005, In Words - Technical Documentation Services and Solutions
#
# Author: Sean Wheller   sean.at.inwords.co.za   http://www.inwords.co.za
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTI
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This program is free software; you can redistribute it and/or modify 
# it under the terms of the GNU General Public License as published by 
# the Free Software Foundation; either version 2 of the License, or 
# (at your  option) any later version.  This program is distributed in 
# the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE.
#
# See the GNU General Public License for more details.
# http://www.gnu.org/copyleft/gpl.html
#
##############
# 
##############
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
    <!-- This file is a customization layer for HTML only -->
    <!-- ======================= -->
    <!-- Imports -->
    <!--
    PATH MAPPED BY 
    catalog.xml href="/usr/share/xml/docbook/stylesheet/nwalsh/current/html/"
    Comment/Uncomment xsl:imports as required.
    -->
    
    <!--<xsl:import href="html-docbook.xsl"/>-->
    <!--<xsl:import href="/usr/share/xml/docbook/stylesheet/nwalsh/html/profile-chunk.xsl"/>-->
    <xsl:import href="/usr/share/xml/docbook/stylesheet/nwalsh/current/html/profile-chunk.xsl"/>
    <xsl:include href="common-cust.xsl"/>
    
    <!-- ======================= -->
    <!-- Parameters -->
    <!-- ======================= -->
    <xsl:param name="generate.legalnotice.link" select="1"/>
    <xsl:param name="chunker.output.indent" select="'yes'"/>
    <xsl:param name="body.font.master" select="12"/>
    <xsl:param name="html.stylesheet" select="'corpstyle.css'"/>
    <xsl:param name="navig.graphics" select="1"/>
	<xsl:param name="navig.graphics.path" select="'../images/navig/'"/>
    <xsl:param name="navig.graphics.extension" select="'.gif'"/>
    <xsl:param name="navig.showtitles" select="1"/>
    
   <!-- Admon Graphics -->
    <xsl:param name="admon.graphics" select="1"/>
    <xsl:param name="admon.textlabel" select="1"/>
	<xsl:param name="admon.graphics.path" select="'../images/admon/'"/>
    <xsl:param name="admon.graphics.extension" select="'.png'"/>
    
    <!-- Callout Graphics -->
    <xsl:param name="callout.unicode" select="1"/>
    <xsl:param name="callout.graphics" select="0"/>
	<xsl:param name="callout.graphics.path" select="'../images/callouts/'"/>
    <xsl:param name="callout.graphics.extension" select="'.png'"/>
  
    <!-- ======================= -->
    <!-- Attribute Sets -->
    <!-- ======================= -->
    
    <!-- ======================= -->
    <!-- Template Customizations -->
    <!-- ======================= -->
    

    
    </xsl:stylesheet>
