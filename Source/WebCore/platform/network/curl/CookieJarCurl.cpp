/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PlatformCookieJar.h"

#if USE(CURL)

#include "Cookie.h"
#include "CurlContext.h"
#include "NotImplemented.h"
#include "URL.h"

#include <wtf/DateMath.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static void readCurlCookieToken(const char*& cookie, String& token)
{
    // Read the next token from a cookie with the Netscape cookie format.
    // Curl separates each token in line with tab character.
    const char* cookieStart = cookie;
    while (cookie && cookie[0] && cookie[0] != '\t')
        cookie++;
    token = String(cookieStart, cookie - cookieStart);
    if (cookie[0] == '\t')
        cookie++;
}

static bool domainMatch(const String& cookieDomain, const String& host)
{
    size_t index = host.find(cookieDomain);

    bool tailMatch = (index != WTF::notFound && index + cookieDomain.length() == host.length());

    // Check if host equals cookie domain.
    if (tailMatch && !index)
        return true;

    // Check if host is a subdomain of the domain in the cookie.
    // Curl uses a '.' in front of domains to indicate it's valid on subdomains.
    if (tailMatch && index > 0 && host[index] == '.')
        return true;

    // Check the special case where host equals the cookie domain, except for a leading '.' in the cookie domain.
    // E.g. cookie domain is .apple.com and host is apple.com. 
    if (cookieDomain[0] == '.' && cookieDomain.find(host) == 1)
        return true;

    return false;
}

static void addMatchingCurlCookie(const char* cookie, const String& domain, const String& path, StringBuilder& cookies, bool httponly)
{
    // Check if the cookie matches domain and path, and is not expired.
    // If so, add it to the list of cookies.
    //
    // Description of the Netscape cookie file format which Curl uses:
    //
    // .netscape.com     TRUE   /  FALSE  946684799   NETSCAPE_ID  100103
    //
    // Each line represents a single piece of stored information. A tab is inserted between each of the fields.
    //
    // From left-to-right, here is what each field represents:
    //
    // domain - The domain that created AND that can read the variable.
    // flag - A TRUE/FALSE value indicating if all machines within a given domain can access the variable. This value is set automatically by the browser, depending on the value you set for domain.
    // path - The path within the domain that the variable is valid for.
    // secure - A TRUE/FALSE value indicating if a secure connection with the domain is needed to access the variable.
    // expiration - The UNIX time that the variable will expire on. UNIX time is defined as the number of seconds since Jan 1, 1970 00:00:00 GMT.
    // name - The name of the variable.
    // value - The value of the variable.

    if (!cookie)
        return;

    String cookieDomain;
    readCurlCookieToken(cookie, cookieDomain);

    // HttpOnly cookie entries begin with "#HttpOnly_".
    if (cookieDomain.startsWith("#HttpOnly_")) {
        if (httponly)
            cookieDomain.remove(0, 10);
        else
            return;
    }

    if (!domainMatch(cookieDomain, domain))
        return;

    String strBoolean;
    readCurlCookieToken(cookie, strBoolean);

    String strPath;
    readCurlCookieToken(cookie, strPath);

    // Check if path matches
    int index = path.find(strPath);
    if (index)
        return;

    String strSecure;
    readCurlCookieToken(cookie, strSecure);

    String strExpires;
    readCurlCookieToken(cookie, strExpires);

    int expires = strExpires.toInt();

    time_t now = 0;
    time(&now);

    // Check if cookie has expired
    if (expires && now > expires)
        return;

    String strName;
    readCurlCookieToken(cookie, strName);

    String strValue;
    readCurlCookieToken(cookie, strValue);

    // The cookie matches, add it to the cookie list.

    if (cookies.length() > 0)
        cookies.append("; ");

    cookies.append(strName);
    cookies.append("=");
    cookies.append(strValue);

}

static String getNetscapeCookieFormat(const URL& url, const String& value)
{
    // Constructs a cookie string in Netscape Cookie file format.

    if (value.isEmpty())
        return "";

    String valueStr;
    if (value.is8Bit())
        valueStr = value;
    else
        valueStr = String::make8BitFrom16BitSource(value.characters16(), value.length());

    Vector<String> attributes;
    valueStr.split(';', false, attributes);

    if (!attributes.size())
        return "";

    // First attribute should be <cookiename>=<cookievalue>
    String cookieName, cookieValue;
    Vector<String>::iterator attribute = attributes.begin();
    if (attribute->contains('=')) {
        Vector<String> nameValuePair;
        attribute->split('=', true, nameValuePair);
        cookieName = nameValuePair[0];
        cookieValue = nameValuePair[1];
    } else {
        // According to RFC6265 we should ignore the entire
        // set-cookie string now, but other browsers appear
        // to treat this as <cookiename>=<empty>
        cookieName = *attribute;
    }
    
    int expires = 0;
    String secure = "FALSE";
    String path = url.baseAsString().substring(url.pathStart());
    if (path.length() > 1 && path.endsWith('/'))
        path.remove(path.length() - 1);
    String domain = url.host();

    // Iterate through remaining attributes
    for (++attribute; attribute != attributes.end(); ++attribute) {
        if (attribute->contains('=')) {
            Vector<String> keyValuePair;
            attribute->split('=', true, keyValuePair);
            String key = keyValuePair[0].stripWhiteSpace();
            String val = keyValuePair[1].stripWhiteSpace();
            if (equalLettersIgnoringASCIICase(key, "expires")) {
                CString dateStr(reinterpret_cast<const char*>(val.characters8()), val.length());
                expires = WTF::parseDateFromNullTerminatedCharacters(dateStr.data()) / WTF::msPerSecond;
            } else if (equalLettersIgnoringASCIICase(key, "max-age"))
                expires = time(0) + val.toInt();
            else if (equalLettersIgnoringASCIICase(key, "domain"))
                domain = val;
            else if (equalLettersIgnoringASCIICase(key, "path"))
                path = val;
        } else {
            String key = attribute->stripWhiteSpace();
            if (equalLettersIgnoringASCIICase(key, "secure"))
                secure = "TRUE";
        }
    }
    
    String allowSubdomains = domain.startsWith('.') ? "TRUE" : "FALSE";
    String expiresStr = String::number(expires);

    int finalStringLength = domain.length() + path.length() + expiresStr.length() + cookieName.length();
    finalStringLength += cookieValue.length() + secure.length() + allowSubdomains.length();
    finalStringLength += 6; // Account for \t separators.
    
    StringBuilder cookieStr;
    cookieStr.reserveCapacity(finalStringLength);
    cookieStr.append(domain);
    cookieStr.append("\t");
    cookieStr.append(allowSubdomains);
    cookieStr.append("\t");
    cookieStr.append(path);
    cookieStr.append("\t");
    cookieStr.append(secure);
    cookieStr.append("\t");
    cookieStr.append(expiresStr);
    cookieStr.append("\t");
    cookieStr.append(cookieName);
    cookieStr.append("\t");
    cookieStr.append(cookieValue);

    return cookieStr.toString();
}

void setCookiesFromDOM(const NetworkStorageSession&, const URL&, const URL& url, const String& value)
{
    const CurlContext& context = CurlContext::singleton();

    CURL* curl = curl_easy_init();

    if (!curl)
        return;

    const char* cookieJarFileName = context.getCookieJarFileName();
    CURLSH* curlsh = context.curlShareHandle();

    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookieJarFileName);
    curl_easy_setopt(curl, CURLOPT_SHARE, curlsh);

    // CURL accepts cookies in either Set-Cookie or Netscape file format.
    // However with Set-Cookie format, there is no way to specify that we
    // should not allow cookies to be read from subdomains, which is the
    // required behavior if the domain field is not explicity specified.
    String cookie = getNetscapeCookieFormat(url, value);

    if (!cookie.is8Bit())
        cookie = String::make8BitFrom16BitSource(cookie.characters16(), cookie.length());

    CString strCookie(reinterpret_cast<const char*>(cookie.characters8()), cookie.length());

    curl_easy_setopt(curl, CURLOPT_COOKIELIST, strCookie.data());

    curl_easy_cleanup(curl);
}

static String cookiesForSession(const NetworkStorageSession&, const URL&, const URL& url, bool httponly)
{
    String cookies;
    CURL* curl = curl_easy_init();

    if (!curl)
        return cookies;

    CURLSH* curlsh = CurlContext::singleton().curlShareHandle();

    curl_easy_setopt(curl, CURLOPT_SHARE, curlsh);

    struct curl_slist* list = 0;
    curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &list);

    if (list) {
        String domain = url.host();
        String path = url.path();
        StringBuilder cookiesBuilder;

        struct curl_slist* item = list;
        while (item) {
            const char* cookie = item->data;
            addMatchingCurlCookie(cookie, domain, path, cookiesBuilder, httponly);
            item = item->next;
        }

        cookies = cookiesBuilder.toString();
        curl_slist_free_all(list);
    }

    curl_easy_cleanup(curl);

    return cookies;
}

String cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    return cookiesForSession(session, firstParty, url, false);
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    return cookiesForSession(session, firstParty, url, true);
}

bool cookiesEnabled(const NetworkStorageSession&, const URL& /*firstParty*/, const URL& /*url*/)
{
    return true;
}

bool getRawCookies(const NetworkStorageSession&, const URL& /*firstParty*/, const URL& /*url*/, Vector<Cookie>& rawCookies)
{
    // FIXME: Not yet implemented
    rawCookies.clear();
    return false; // return true when implemented
}

void deleteCookie(const NetworkStorageSession&, const URL&, const String&)
{
    // FIXME: Not yet implemented
}

void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>&)
{
    // FIXME: Not yet implemented
}

void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>&)
{
    // FIXME: Not yet implemented
}

void deleteAllCookies(const NetworkStorageSession&)
{
    // FIXME: Not yet implemented
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point)
{
    // FIXME: Not yet implemented
}

}

#endif
