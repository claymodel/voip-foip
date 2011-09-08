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
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fo="http://www.w3.org/1999/XSL/Format" version="1.0">
    
    <!-- This file is a customization layer for FO only -->
    <!-- ======================= -->
    <!-- Imports -->
    <!--<xsl:import href="/usr/share/xml/docbook/stylesheet/nwalsh/fo/profile-docbook.xsl"/>-->
    <xsl:import href="/usr/share/xml/docbook/stylesheet/nwalsh/current/fo/profile-docbook.xsl"/>
    <xsl:include href="common-cust.xsl"/>
    
    <!-- ======================= -->
    <!-- Parameters -->
    <!-- ======================= -->
    
    <xsl:param name="fop.extensions" select="1"/>
    <xsl:param name="paper.type" select="'A4'"/>
    <xsl:param name="double.sided" select="0"/>
    <xsl:param name="header.rule" select="1"/>
    <xsl:param name="footer.rule" select="1"/>
    <xsl:param name="body.font.master" select="'11'"/>
    <xsl:param name="section.autolabel" select="'0'"/>
    
  <!-- Admon Graphics -->
    <xsl:param name="admon.graphics" select="0"/>
    <xsl:param name="admon.textlabel" select="0"/>
    <xsl:param name="admon.graphics.path" select="'../images/admon/'"/>
    <xsl:param name="admon.graphics.extension" select="'.png'"/>
    
    <!-- Callout Graphics -->
    <xsl:param name="callout.unicode" select="0"/>
    <xsl:param name="callout.graphics" select="0"/>
    <xsl:param name="callout.graphics.path" select="'../images/callouts/'"/>
    <xsl:param name="callout.graphics.extension" select="'.png'"/>
  
    <!-- ======================= -->
    <!-- Attribute Sets -->
    <!-- ======================= -->
    
    <!-- Page Break -->
    <!--<xsl:attribute-set name="section.level1.properties">
      <xsl:attribute name="break-before">page</xsl:attribute>
    </xsl:attribute-set>-->
    
    <!-- Header Design Formatting-->
    <xsl:attribute-set name="header.content.properties">
        <xsl:attribute name="font-family">Helvetica</xsl:attribute>
        <xsl:attribute name="font-size">9pt</xsl:attribute>
    </xsl:attribute-set>
    
    <!-- ======================= -->
    <!-- Templates -->
    <!-- ======================= -->
    <!-- Lists -->
    <xsl:attribute-set name="list.item.spacing">
        <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-before.minimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>
    </xsl:attribute-set>
    
    <xsl:attribute-set name="list.block.spacing">
        <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-before.minimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>
        <xsl:attribute name="space-after.optimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-after.minimum">0.5em</xsl:attribute>
        <xsl:attribute name="space-after.maximum">0.5em</xsl:attribute>
    </xsl:attribute-set>
    
    <!-- Running Header Footer Content-->
    <xsl:template name="header.content">
        <xsl:param name="pageclass" select="''"/>
        <xsl:param name="sequence" select="''"/>
        <xsl:param name="position" select="''"/>
        <xsl:param name="gentext-key" select="''"/>
        <xsl:variable name="candidate">
            <!-- sequence can be odd, even, first, blank -->
            <!-- position can be left, center, right -->
            <xsl:choose>
                <xsl:when test="$sequence = 'odd' and $position = 'left'">
                    <fo:retrieve-marker retrieve-class-name="section.head.marker"
                        retrieve-position="first-including-carryover" retrieve-boundary="page"/>
                </xsl:when>
                <xsl:when test="$sequence = 'odd' and $position = 'center'">
                    <xsl:call-template name="draft.text"/>
                </xsl:when>
                <xsl:when test="$sequence = 'odd' and $position = 'right'">
                    <fo:page-number/>
                </xsl:when>
                <xsl:when test="$sequence = 'even' and $position = 'left'">
                    <fo:page-number/>
                </xsl:when>
                <xsl:when test="$sequence = 'even' and $position = 'center'">
                    <xsl:call-template name="draft.text"/>
                </xsl:when>
                <xsl:when test="$sequence = 'even' and $position = 'right'">
                    <xsl:apply-templates select="." mode="titleabbrev.markup"/>
                </xsl:when>
                <xsl:when test="$sequence = 'first' and $position = 'left'"/>
                <xsl:when test="$sequence = 'first' and $position = 'right'"/>
                <xsl:when test="$sequence = 'first' and $position = 'center'">
                    <xsl:value-of select="ancestor-or-self::book/bookinfo/corpauthor"/>
                </xsl:when>
                <xsl:when test="$sequence = 'blank' and $position = 'left'">
                    <fo:page-number/>
                </xsl:when>
                <xsl:when test="$sequence = 'blank' and $position = 'center'">
                    <xsl:text>This page intentionally left blank</xsl:text>
                </xsl:when>
                <xsl:when test="$sequence = 'blank' and $position = 'right'"/>
            </xsl:choose>
        </xsl:variable>
        <!-- Does runtime parameter turn off blank page headers? -->
        <xsl:choose>
            <xsl:when test="$sequence='blank' and $headers.on.blank.pages=0">
                <!-- no output -->
            </xsl:when>
            <!-- titlepages have no headers -->
            <xsl:when test="$pageclass = 'titlepage'">
                <!-- no output -->
            </xsl:when>
            <xsl:otherwise>
                <xsl:copy-of select="$candidate"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>
    
    <!-- Switch OFF Running Footer -->
    <xsl:template name="footer.content"/>
    
    <!-- Page Break use &lt?pagebreak?;&gt; to insert a page break -->
    <xsl:template match="processing-instruction('pagebreak')">
        <fo:block break-after="page"/>
    </xsl:template>
    
</xsl:stylesheet>
