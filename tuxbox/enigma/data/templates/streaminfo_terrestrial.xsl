<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/">
<html>
<body>
	<h2>Stream Information</h2>
	<table border="0">
		<tr><td>Service Name:</td><td><xsl:value-of select="streaminfo/service/name"/></td></tr>
		<tr><td>Service Provider:</td><td><xsl:value-of select="streaminfo/provider"/></td></tr>
		<tr><td>Service Reference:</td><td><xsl:value-of select="streaminfo/service/reference"/></td></tr>
		<tr><td>VPID:</td><td><xsl:value-of select="streaminfo/vpid"/></td></tr>
		<tr><td>APID:</td><td><xsl:value-of select="streaminfo/apid"/></td></tr>
		<tr><td>PCRPID:</td><td><xsl:value-of select="streaminfo/pcrpid"/></td></tr>
		<tr><td>TPID:</td><td><xsl:value-of select="streaminfo/tpid"/></td></tr>
		<tr><td>TSID:</td><td><xsl:value-of select="streaminfo/tsid"/></td></tr>
		<tr><td>ONID:</td><td><xsl:value-of select="streaminfo/onid"/></td></tr>
		<tr><td>SID:</td><td><xsl:value-of select="streaminfo/sid"/></td></tr>
		<tr><td>PMT:</td><td><xsl:value-of select="streaminfo/pmt"/></td></tr>
		<tr><td>Video Format:</td><td><xsl:value-of select="streaminfo/video_format"/></td></tr>
		<tr><td>Namespace:</td><td><xsl:value-of select="streaminfo/namespace"/></td></tr>
		<tr><td>Supported Crypto Systems:</td><td><xsl:value-of select="streaminfo/supported_crypt_systems"/></td></tr>
		<tr><td>Used Crypto Systems:</td><td><xsl:value-of select="streaminfo/used_crypt_systems"/></td></tr>
		<tr><td>Center Frequency:</td><td><xsl:value-of select="streaminfo/centerfrequency"/> MHz</td></tr>
		<tr><td>Hierarchy Info:</td><td><xsl:value-of select="streaminfo/hierarchyinfo"/></td></tr>
		<tr><td>Inversion:</td><td><xsl:value-of select="streaminfo/inversion"/></td></tr>
		<tr><td>Bandwidth:</td><td><xsl:value-of select="streaminfo/bandwidth"/></td></tr>
		<tr><td>Constellation:</td><td><xsl:value-of select="streaminfo/constellation"/></td></tr>
		<tr><td>Guard Interval:</td><td><xsl:value-of select="streaminfo/guardinterval"/></td></tr>
		<tr><td>Transmission Mode:</td><td><xsl:value-of select="streaminfo/transmission"/></td></tr>
		<tr><td>Code Rate LP:</td><td><xsl:value-of select="streaminfo/coderatelp"/></td></tr>
		<tr><td>Code Rate HP:</td><td><xsl:value-of select="streaminfo/coderatehp"/></td></tr>
	</table>
</body>
</html>
</xsl:template>
</xsl:stylesheet>
