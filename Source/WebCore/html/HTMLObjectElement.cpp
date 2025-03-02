/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTMLObjectElement.h"

#include "Attribute.h"
#include "CSSValueKeywords.h"
#include "CachedImage.h"
#include "ElementIterator.h"
#include "FormDataList.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLDocument.h"
#include "HTMLFormElement.h"
#include "HTMLImageLoader.h"
#include "HTMLMetaElement.h"
#include "HTMLNames.h"
#include "HTMLParamElement.h"
#include "HTMLParserIdioms.h"
#include "MIMETypeRegistry.h"
#include "NodeList.h"
#include "Page.h"
#include "PluginViewBase.h"
#include "RenderEmbeddedObject.h"
#include "RenderImage.h"
#include "RenderWidget.h"
#include "Settings.h"
#include "SubframeLoader.h"
#include "Text.h"
#include "Widget.h"
#include <wtf/Ref.h>

#if PLATFORM(IOS)
#include "RuntimeApplicationChecks.h"
#include <wtf/spi/darwin/dyldSPI.h>
#endif

namespace WebCore {

using namespace HTMLNames;

inline HTMLObjectElement::HTMLObjectElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form, bool createdByParser)
    : HTMLPlugInImageElement(tagName, document, createdByParser)
    , FormAssociatedElement(form)
    , m_docNamedItem(true)
    , m_useFallbackContent(false)
{
    ASSERT(hasTagName(objectTag));
}

inline HTMLObjectElement::~HTMLObjectElement()
{
}

Ref<HTMLObjectElement> HTMLObjectElement::create(const QualifiedName& tagName, Document& document, HTMLFormElement* form, bool createdByParser)
{
    return adoptRef(*new HTMLObjectElement(tagName, document, form, createdByParser));
}

RenderWidget* HTMLObjectElement::renderWidgetLoadingPlugin() const
{
    // Needs to load the plugin immediatedly because this function is called
    // when JavaScript code accesses the plugin.
    // FIXME: <rdar://16893708> Check if dispatching events here is safe.
    document().updateLayoutIgnorePendingStylesheets(Document::RunPostLayoutTasks::Synchronously);
    return renderWidget(); // This will return 0 if the renderer is not a RenderWidget.
}

bool HTMLObjectElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == borderAttr)
        return true;
    return HTMLPlugInImageElement::isPresentationAttribute(name);
}

void HTMLObjectElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStyleProperties& style)
{
    if (name == borderAttr)
        applyBorderAttributeToStyle(value, style);
    else
        HTMLPlugInImageElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLObjectElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    bool invalidateRenderer = false;

    if (name == formAttr)
        formAttributeChanged();
    else if (name == typeAttr) {
        m_serviceType = value.string().left(value.find(';')).convertToASCIILowercase();
        invalidateRenderer = !hasAttributeWithoutSynchronization(classidAttr);
        setNeedsWidgetUpdate(true);
    } else if (name == dataAttr) {
        m_url = stripLeadingAndTrailingHTMLSpaces(value);
        document().updateStyleIfNeeded();
        if (isImageType() && renderer()) {
            if (!m_imageLoader)
                m_imageLoader = std::make_unique<HTMLImageLoader>(*this);
            m_imageLoader->updateFromElementIgnoringPreviousError();
        }
        invalidateRenderer = !hasAttributeWithoutSynchronization(classidAttr);
        setNeedsWidgetUpdate(true);
    } else if (name == classidAttr) {
        invalidateRenderer = true;
        setNeedsWidgetUpdate(true);
    } else
        HTMLPlugInImageElement::parseAttribute(name, value);

    if (!invalidateRenderer || !isConnected() || !renderer())
        return;

    clearUseFallbackContent();
    invalidateStyleAndRenderersForSubtree();
}

static void mapDataParamToSrc(Vector<String>& paramNames, Vector<String>& paramValues)
{
    // Some plugins don't understand the "data" attribute of the OBJECT tag (i.e. Real and WMP require "src" attribute).
    bool foundSrcParam = false;
    String dataParamValue;
    for (unsigned i = 0; i < paramNames.size(); ++i) {
        if (equalLettersIgnoringASCIICase(paramNames[i], "src"))
            foundSrcParam = true;
        else if (equalLettersIgnoringASCIICase(paramNames[i], "data"))
            dataParamValue = paramValues[i];
    }
    if (!foundSrcParam && !dataParamValue.isNull()) {
        paramNames.append(ASCIILiteral("src"));
        paramValues.append(WTFMove(dataParamValue));
    }
}

#if PLATFORM(IOS)
static bool shouldNotPerformURLAdjustment()
{
    static bool shouldNotPerformURLAdjustment = IOSApplication::isNASAHD() && dyld_get_program_sdk_version() < DYLD_IOS_VERSION_5_0;
    return shouldNotPerformURLAdjustment;
}
#endif

// FIXME: This function should not deal with url or serviceType!
void HTMLObjectElement::parametersForPlugin(Vector<String>& paramNames, Vector<String>& paramValues, String& url, String& serviceType)
{
    HashSet<StringImpl*, ASCIICaseInsensitiveHash> uniqueParamNames;
    String urlParameter;
    
    // Scan the PARAM children and store their name/value pairs.
    // Get the URL and type from the params if we don't already have them.
    for (auto& param : childrenOfType<HTMLParamElement>(*this)) {
        String name = param.name();
        if (name.isEmpty())
            continue;

        uniqueParamNames.add(name.impl());
        paramNames.append(param.name());
        paramValues.append(param.value());

        // FIXME: url adjustment does not belong in this function.
        if (url.isEmpty() && urlParameter.isEmpty() && (equalLettersIgnoringASCIICase(name, "src") || equalLettersIgnoringASCIICase(name, "movie") || equalLettersIgnoringASCIICase(name, "code") || equalLettersIgnoringASCIICase(name, "url")))
            urlParameter = stripLeadingAndTrailingHTMLSpaces(param.value());
        // FIXME: serviceType calculation does not belong in this function.
        if (serviceType.isEmpty() && equalLettersIgnoringASCIICase(name, "type")) {
            serviceType = param.value();
            size_t pos = serviceType.find(';');
            if (pos != notFound)
                serviceType = serviceType.left(pos);
        }
    }
    
    // When OBJECT is used for an applet via Sun's Java plugin, the CODEBASE attribute in the tag
    // points to the Java plugin itself (an ActiveX component) while the actual applet CODEBASE is
    // in a PARAM tag. See <http://java.sun.com/products/plugin/1.2/docs/tags.html>. This means
    // we have to explicitly suppress the tag's CODEBASE attribute if there is none in a PARAM,
    // else our Java plugin will misinterpret it. [4004531]
    String codebase;
    if (MIMETypeRegistry::isJavaAppletMIMEType(serviceType)) {
        codebase = "codebase";
        uniqueParamNames.add(codebase.impl()); // pretend we found it in a PARAM already
    }
    
    // Turn the attributes of the <object> element into arrays, but don't override <param> values.
    if (hasAttributes()) {
        for (const Attribute& attribute : attributesIterator()) {
            const AtomicString& name = attribute.name().localName();
            if (!uniqueParamNames.contains(name.impl())) {
                paramNames.append(name.string());
                paramValues.append(attribute.value().string());
            }
        }
    }
    
    mapDataParamToSrc(paramNames, paramValues);
    
    // HTML5 says that an object resource's URL is specified by the object's data
    // attribute, not by a param element. However, for compatibility, allow the
    // resource's URL to be given by a param named "src", "movie", "code" or "url"
    // if we know that resource points to a plug-in.
#if PLATFORM(IOS)
    if (shouldNotPerformURLAdjustment())
        return;
#endif

    if (url.isEmpty() && !urlParameter.isEmpty()) {
        SubframeLoader& loader = document().frame()->loader().subframeLoader();
        if (loader.resourceWillUsePlugin(urlParameter, serviceType))
            url = urlParameter;
    }
}

    
bool HTMLObjectElement::hasFallbackContent() const
{
    for (Node* child = firstChild(); child; child = child->nextSibling()) {
        // Ignore whitespace-only text, and <param> tags, any other content is fallback content.
        if (is<Text>(*child)) {
            if (!downcast<Text>(*child).containsOnlyWhitespace())
                return true;
        } else if (!is<HTMLParamElement>(*child))
            return true;
    }
    return false;
}
    
bool HTMLObjectElement::shouldAllowQuickTimeClassIdQuirk()
{
    // This site-specific hack maintains compatibility with Mac OS X Wiki Server,
    // which embeds QuickTime movies using an object tag containing QuickTime's
    // ActiveX classid. Treat this classid as valid only if OS X Server's unique
    // 'generator' meta tag is present. Only apply this quirk if there is no
    // fallback content, which ensures the quirk will disable itself if Wiki
    // Server is updated to generate an alternate embed tag as fallback content.

    if (!document().page()
        || !document().page()->settings().needsSiteSpecificQuirks()
        || hasFallbackContent()
        || !equalLettersIgnoringASCIICase(attributeWithoutSynchronization(classidAttr), "clsid:02bf25d5-8c17-4b23-bc80-d3488abddc6b"))
        return false;

    for (auto& metaElement : descendantsOfType<HTMLMetaElement>(document())) {
        if (equalLettersIgnoringASCIICase(metaElement.name(), "generator") && metaElement.content().startsWith("Mac OS X Server Web Services Server", false))
            return true;
    }

    return false;
}
    
bool HTMLObjectElement::hasValidClassId()
{
    if (MIMETypeRegistry::isJavaAppletMIMEType(serviceType()) && attributeWithoutSynchronization(classidAttr).startsWith("java:", false))
        return true;
    
    if (shouldAllowQuickTimeClassIdQuirk())
        return true;

    // HTML5 says that fallback content should be rendered if a non-empty
    // classid is specified for which the UA can't find a suitable plug-in.
    return attributeWithoutSynchronization(classidAttr).isEmpty();
}

// FIXME: This should be unified with HTMLEmbedElement::updateWidget and
// moved down into HTMLPluginImageElement.cpp
void HTMLObjectElement::updateWidget(CreatePlugins createPlugins)
{
    ASSERT(!renderEmbeddedObject()->isPluginUnavailable());
    ASSERT(needsWidgetUpdate());
    setNeedsWidgetUpdate(false);
    // FIXME: This should ASSERT isFinishedParsingChildren() instead.
    if (!isFinishedParsingChildren())
        return;

    // FIXME: I'm not sure it's ever possible to get into updateWidget during a
    // removal, but just in case we should avoid loading the frame to prevent
    // security bugs.
    if (!SubframeLoadingDisabler::canLoadFrame(*this))
        return;

    String url = this->url();
    String serviceType = this->serviceType();

    // FIXME: These should be joined into a PluginParameters class.
    Vector<String> paramNames;
    Vector<String> paramValues;
    parametersForPlugin(paramNames, paramValues, url, serviceType);

    // Note: url is modified above by parametersForPlugin.
    if (!allowedToLoadFrameURL(url))
        return;

    // FIXME: It's sadness that we have this special case here.
    //        See http://trac.webkit.org/changeset/25128 and
    //        plugins/netscape-plugin-setwindow-size.html
    if (createPlugins == CreatePlugins::No && wouldLoadAsPlugIn(url, serviceType)) {
        // Ensure updateWidget() is called again during layout to create the Netscape plug-in.
        setNeedsWidgetUpdate(true);
        return;
    }

    Ref<HTMLObjectElement> protectedThis(*this); // beforeload and plugin loading can make arbitrary DOM mutations.
    bool beforeLoadAllowedLoad = guardedDispatchBeforeLoadEvent(url);
    if (!renderer()) // Do not load the plugin if beforeload removed this element or its renderer.
        return;

    bool success = beforeLoadAllowedLoad && hasValidClassId() && allowedToLoadFrameURL(url);
    if (success)
        success = requestObject(url, serviceType, paramNames, paramValues);
    if (!success && hasFallbackContent())
        renderFallbackContent();
}

Node::InsertionNotificationRequest HTMLObjectElement::insertedInto(ContainerNode& insertionPoint)
{
    HTMLPlugInImageElement::insertedInto(insertionPoint);
    FormAssociatedElement::insertedInto(insertionPoint);
    return InsertionShouldCallFinishedInsertingSubtree;
}

void HTMLObjectElement::finishedInsertingSubtree()
{
    resetFormOwner();
}

void HTMLObjectElement::removedFrom(ContainerNode& insertionPoint)
{
    HTMLPlugInImageElement::removedFrom(insertionPoint);
    FormAssociatedElement::removedFrom(insertionPoint);
}

void HTMLObjectElement::childrenChanged(const ChildChange& change)
{
    updateDocNamedItem();
    if (isConnected() && !useFallbackContent()) {
        setNeedsWidgetUpdate(true);
        invalidateStyleForSubtree();
    }
    HTMLPlugInImageElement::childrenChanged(change);
}

bool HTMLObjectElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == dataAttr || attribute.name() == codebaseAttr || (attribute.name() == usemapAttr && attribute.value().string()[0] != '#') || HTMLPlugInImageElement::isURLAttribute(attribute);
}

const AtomicString& HTMLObjectElement::imageSourceURL() const
{
    return attributeWithoutSynchronization(dataAttr);
}

void HTMLObjectElement::renderFallbackContent()
{
    if (useFallbackContent())
        return;
    
    if (!isConnected())
        return;

    invalidateStyleAndRenderersForSubtree();

    // Before we give up and use fallback content, check to see if this is a MIME type issue.
    auto* loader = imageLoader();
    if (loader && loader->image() && loader->image()->status() != CachedResource::LoadError) {
        m_serviceType = loader->image()->response().mimeType();
        if (!isImageType()) {
            // If we don't think we have an image type anymore, then clear the image from the loader.
            loader->clearImage();
            return;
        }
    }

    m_useFallbackContent = true;

    // This was added to keep Acid 2 non-flaky. A style recalc is required to make fallback resources load.
    // Without forcing, this may happen after all the other resources have been loaded and the document is already
    // considered complete. FIXME: Would be better to address this with incrementLoadEventDelayCount instead
    // or disentangle loading from style entirely.
    document().updateStyleIfNeeded();
}

// FIXME: This should be removed, all callers are almost certainly wrong.
static bool isRecognizedTagName(const QualifiedName& tagName)
{
    static NeverDestroyed<HashSet<AtomicStringImpl*>> tagList;
    if (tagList.get().isEmpty()) {
        auto* tags = HTMLNames::getHTMLTags();
        for (size_t i = 0; i < HTMLNames::HTMLTagsCount; i++) {
            if (*tags[i] == bgsoundTag
                || *tags[i] == commandTag
                || *tags[i] == detailsTag
                || *tags[i] == figcaptionTag
                || *tags[i] == figureTag
                || *tags[i] == summaryTag
                || *tags[i] == trackTag) {
                // Even though we have atoms for these tags, we don't want to
                // treat them as "recognized tags" for the purpose of parsing
                // because that changes how we parse documents.
                continue;
            }
            tagList.get().add(tags[i]->localName().impl());
        }
    }
    return tagList.get().contains(tagName.localName().impl());
}

void HTMLObjectElement::updateDocNamedItem()
{
    // The rule is "<object> elements with no children other than
    // <param> elements, unknown elements and whitespace can be
    // found by name in a document, and other <object> elements cannot."
    bool wasNamedItem = m_docNamedItem;
    bool isNamedItem = true;
    Node* child = firstChild();
    while (child && isNamedItem) {
        if (is<Element>(*child)) {
            Element& element = downcast<Element>(*child);
            // FIXME: Use of isRecognizedTagName is almost certainly wrong here.
            if (isRecognizedTagName(element.tagQName()) && !element.hasTagName(paramTag))
                isNamedItem = false;
        } else if (is<Text>(*child)) {
            if (!downcast<Text>(*child).containsOnlyWhitespace())
                isNamedItem = false;
        } else
            isNamedItem = false;
        child = child->nextSibling();
    }
    if (isNamedItem != wasNamedItem && isConnected() && !isInShadowTree() && is<HTMLDocument>(document())) {
        HTMLDocument& document = downcast<HTMLDocument>(this->document());

        const AtomicString& id = getIdAttribute();
        if (!id.isEmpty()) {
            if (isNamedItem)
                document.addDocumentNamedItem(*id.impl(), *this);
            else
                document.removeDocumentNamedItem(*id.impl(), *this);
        }

        const AtomicString& name = getNameAttribute();
        if (!name.isEmpty() && id != name) {
            if (isNamedItem)
                document.addDocumentNamedItem(*name.impl(), *this);
            else
                document.removeDocumentNamedItem(*name.impl(), *this);
        }
    }
    m_docNamedItem = isNamedItem;
}

bool HTMLObjectElement::containsJavaApplet() const
{
    if (MIMETypeRegistry::isJavaAppletMIMEType(attributeWithoutSynchronization(typeAttr)))
        return true;

    for (auto& child : childrenOfType<Element>(*this)) {
        if (child.hasTagName(paramTag) && equalLettersIgnoringASCIICase(child.getNameAttribute(), "type")
            && MIMETypeRegistry::isJavaAppletMIMEType(child.attributeWithoutSynchronization(valueAttr).string()))
            return true;
        if (child.hasTagName(objectTag) && downcast<HTMLObjectElement>(child).containsJavaApplet())
            return true;
        if (child.hasTagName(appletTag))
            return true;
    }
    
    return false;
}

void HTMLObjectElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    HTMLPlugInImageElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document().completeURL(attributeWithoutSynchronization(dataAttr)));

    // FIXME: Passing a string that starts with "#" to the completeURL function does
    // not seem like it would work. The image element has similar but not identical code.
    const AtomicString& useMap = attributeWithoutSynchronization(usemapAttr);
    if (useMap.startsWith('#'))
        addSubresourceURL(urls, document().completeURL(useMap));
}

void HTMLObjectElement::didMoveToNewDocument(Document& oldDocument, Document& newDocument)
{
    FormAssociatedElement::didMoveToNewDocument(oldDocument);
    HTMLPlugInImageElement::didMoveToNewDocument(oldDocument, newDocument);
}

bool HTMLObjectElement::appendFormData(FormDataList& encoding, bool)
{
    if (name().isEmpty())
        return false;

    // Use PluginLoadingPolicy::DoNotLoad here or it would fire JS events synchronously
    // which would not be safe here.
    auto* widget = pluginWidget(PluginLoadingPolicy::DoNotLoad);
    if (!is<PluginViewBase>(widget))
        return false;
    String value;
    if (!downcast<PluginViewBase>(*widget).getFormValue(value))
        return false;
    encoding.appendData(name(), value);
    return true;
}

bool HTMLObjectElement::canContainRangeEndPoint() const
{
    // Call through to HTMLElement because we need to skip HTMLPlugInElement
    // when calling through to the derived class since returns false unconditionally.
    // An object element with fallback content should basically be treated like
    // a generic HTML element.
    return m_useFallbackContent && HTMLElement::canContainRangeEndPoint();
}

}
