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
    
    <!-- This file is a common customization layer -->
    <!-- ======================= -->
    
    <!-- ======================= -->
    <!-- Parameters -->
    <!-- ======================= -->
    
    <!-- Extensions -->
     <xsl:param name="use.extensions" select="1"/>
    <xsl:param name="saxon.extensions" select="1"/>
    <xsl:param name="tablecolumns.extension" select="1"/>
    <xsl:param name="callouts.extensions" select="1"/> 
    
    <!-- General Formatting-->
    <xsl:param name="draft.mode" select="'no'"/>
  <!--<xsl:param name="variablelist.as.blocks" select="1"/> Use <?dbfo term-width=".75in"?> -->
    <xsl:param name="shade.verbatim" select="1"/>
    <xsl:param name="hyphenate">false</xsl:param>
    <xsl:param name="alignment">left</xsl:param>
    
    <!-- Cross References -->
    <xsl:param name="insert.xref.page.number" select="'yes'"/>
    <xsl:param name="xref.with.number.and.title" select="1"/>
    
    <!-- Indexes -->
    <xsl:param name="generate.index" select="0"/>
    
    <!-- Glossaries -->
    <xsl:param name="glossterm.auto.link" select="1"/>
    <xsl:param name="firstterm.only.link" select="1"/>
    <xsl:param name="glossary.collection">../common/glossary.xml</xsl:param>
    <xsl:param name="glossentry.show.acronym" select="'primary'"/>
    
    <!-- Bibliographies -->
    <xsl:param name="bibliography.collection">../common/biblio.xml</xsl:param>
    <xsl:param name="bibliography.numbered" select="1"/>
    
    <!-- Captions -->
  <xsl:param name="formal.procedures" select="0"/>
    <xsl:param name="formal.title.placement">
        figure before
        example before 
        equation before 
        table before 
        procedure before
    </xsl:param>
    <!-- TOC -->
  <xsl:param name="generate.toc">
    appendix  toc,title
    article/appendix  nop
    article   toc,title
    book      toc,title
    chapter   title
    part      toc,title
    preface   toc,title
    qandadiv  toc
    qandaset  toc
    reference toc,title
    set       toc,title
  </xsl:param>
    <!-- ======================= -->
    <!-- Templates -->
    <!-- ======================= -->
  <!-- Section level heading styles -->
  <xsl:attribute-set name="section.title.level1.properties">
    <xsl:attribute name="font-size">
      <xsl:value-of select="$body.font.master * 1.6"/>
      <xsl:text>pt</xsl:text>
    </xsl:attribute>
    <xsl:attribute name="border-top">0.5pt solid black</xsl:attribute>
    <xsl:attribute name="padding-top">20pt</xsl:attribute>
    <xsl:attribute name="padding-bottom">12pt</xsl:attribute>
  </xsl:attribute-set>
    <xsl:attribute-set name="section.title.level1.properties">
      
    </xsl:attribute-set>
  <xsl:attribute-set name="section.title.level2.properties">
    <xsl:attribute name="font-size">
      <xsl:value-of select="$body.font.master * 1.4"/>
      <xsl:text>pt</xsl:text>
    </xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="section.title.level3.properties">
    <xsl:attribute name="font-size">
      <xsl:value-of select="$body.font.master * 1.3"/>
      <xsl:text>pt</xsl:text>
    </xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="section.title.level4.properties">
    <xsl:attribute name="font-size">
      <xsl:value-of select="$body.font.master * 1.2"/>
      <xsl:text>pt</xsl:text>
    </xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="section.title.level5.properties">
    <xsl:attribute name="font-size">
      <xsl:value-of select="$body.font.master * 1.1"/>
      <xsl:text>pt</xsl:text>
    </xsl:attribute>
  </xsl:attribute-set>
    <!-- Inline Formatting -->
    <xsl:template match="guibutton">
        <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    <xsl:template match="filename">
        <xsl:call-template name="inline.boldmonoseq"/>
    </xsl:template>
    <xsl:template match="interface">
      <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    <xsl:template match="option">
      <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    <xsl:template match="guilabel">
      <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    <xsl:template match="keycap">
      <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    <xsl:template match="application">
      <xsl:call-template name="inline.boldseq"/>
    </xsl:template>
    
</xsl:stylesheet>
