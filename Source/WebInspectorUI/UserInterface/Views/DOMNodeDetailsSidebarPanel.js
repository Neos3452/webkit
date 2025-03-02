/*
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.DOMNodeDetailsSidebarPanel = class DOMNodeDetailsSidebarPanel extends WebInspector.DOMDetailsSidebarPanel
{
    constructor()
    {
        super("dom-node-details", WebInspector.UIString("Node"));

        this._eventListenerGroupingMethodSetting = new WebInspector.Setting("dom-node-event-listener-grouping-method", WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod.Event);

        this.element.classList.add("dom-node");

        this._nodeRemoteObject = null;
    }

    // Protected

    initialLayout()
    {
        super.initialLayout();

        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.AttributeModified, this._attributesChanged, this);
        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.AttributeRemoved, this._attributesChanged, this);
        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.CharacterDataModified, this._characterDataModified, this);
        WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.CustomElementStateChanged, this._customElementStateChanged, this);

        this._identityNodeTypeRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Type"));
        this._identityNodeNameRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Name"));
        this._identityNodeValueRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Value"));
        this._identityNodeContentSecurityPolicyHashRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("CSP Hash"));

        var identityGroup = new WebInspector.DetailsSectionGroup([this._identityNodeTypeRow, this._identityNodeNameRow, this._identityNodeValueRow, this._identityNodeContentSecurityPolicyHashRow]);
        var identitySection = new WebInspector.DetailsSection("dom-node-identity", WebInspector.UIString("Identity"), [identityGroup]);

        this._attributesDataGridRow = new WebInspector.DetailsSectionDataGridRow(null, WebInspector.UIString("No Attributes"));

        var attributesGroup = new WebInspector.DetailsSectionGroup([this._attributesDataGridRow]);
        var attributesSection = new WebInspector.DetailsSection("dom-node-attributes", WebInspector.UIString("Attributes"), [attributesGroup]);

        this._propertiesRow = new WebInspector.DetailsSectionRow;

        var propertiesGroup = new WebInspector.DetailsSectionGroup([this._propertiesRow]);
        var propertiesSection = new WebInspector.DetailsSection("dom-node-properties", WebInspector.UIString("Properties"), [propertiesGroup]);

        let eventListenersFilterElement = useSVGSymbol("Images/FilterFieldGlyph.svg", "filter", WebInspector.UIString("Grouping Method"));

        let eventListenersGroupMethodSelectElement = eventListenersFilterElement.appendChild(document.createElement("select"));
        eventListenersGroupMethodSelectElement.addEventListener("change", (event) => {
            this._eventListenerGroupingMethodSetting.value = eventListenersGroupMethodSelectElement.value;

            this._refreshEventListeners();
        });

        function createOption(text, value) {
            let optionElement = eventListenersGroupMethodSelectElement.appendChild(document.createElement("option"));
            optionElement.value = value;
            optionElement.textContent = text;
        }

        createOption(WebInspector.UIString("Group by Event"), WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod.Event);
        createOption(WebInspector.UIString("Group by Node"), WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod.Node);

        eventListenersGroupMethodSelectElement.value = this._eventListenerGroupingMethodSetting.value;

        this._eventListenersSectionGroup = new WebInspector.DetailsSectionGroup;
        let eventListenersSection = new WebInspector.DetailsSection("dom-node-event-listeners", WebInspector.UIString("Event Listeners"), [this._eventListenersSectionGroup], eventListenersFilterElement);

        this.contentView.element.appendChild(identitySection.element);
        this.contentView.element.appendChild(attributesSection.element);
        this.contentView.element.appendChild(propertiesSection.element);
        this.contentView.element.appendChild(eventListenersSection.element);

        if (this._accessibilitySupported()) {
            this._accessibilityEmptyRow = new WebInspector.DetailsSectionRow(WebInspector.UIString("No Accessibility Information"));
            this._accessibilityNodeActiveDescendantRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Shared Focus"));
            this._accessibilityNodeBusyRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Busy"));
            this._accessibilityNodeCheckedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Checked"));
            this._accessibilityNodeChildrenRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Children"));
            this._accessibilityNodeControlsRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Controls"));
            this._accessibilityNodeCurrentRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Current"));
            this._accessibilityNodeDisabledRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Disabled"));
            this._accessibilityNodeExpandedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Expanded"));
            this._accessibilityNodeFlowsRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Flows"));
            this._accessibilityNodeFocusedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Focused"));
            this._accessibilityNodeHeadingLevelRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Heading Level"));
            this._accessibilityNodehierarchyLevelRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Hierarchy Level"));
            this._accessibilityNodeIgnoredRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Ignored"));
            this._accessibilityNodeInvalidRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Invalid"));
            this._accessibilityNodeLiveRegionStatusRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Live"));
            this._accessibilityNodeMouseEventRow = new WebInspector.DetailsSectionSimpleRow("");
            this._accessibilityNodeLabelRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Label"));
            this._accessibilityNodeOwnsRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Owns"));
            this._accessibilityNodeParentRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Parent"));
            this._accessibilityNodePressedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Pressed"));
            this._accessibilityNodeReadonlyRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Readonly"));
            this._accessibilityNodeRequiredRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Required"));
            this._accessibilityNodeRoleRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Role"));
            this._accessibilityNodeSelectedRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Selected"));
            this._accessibilityNodeSelectedChildrenRow = new WebInspector.DetailsSectionSimpleRow(WebInspector.UIString("Selected Items"));

            this._accessibilityGroup = new WebInspector.DetailsSectionGroup([this._accessibilityEmptyRow]);
            var accessibilitySection = new WebInspector.DetailsSection("dom-node-accessibility", WebInspector.UIString("Accessibility"), [this._accessibilityGroup]);

            this.contentView.element.appendChild(accessibilitySection.element);
        }
    }

    layout()
    {
        super.layout();

        if (!this.domNode)
            return;

        this._refreshIdentity();
        this._refreshAttributes();
        this._refreshProperties();
        this._refreshEventListeners();
        this._refreshAccessibility();
    }

    sizeDidChange()
    {
        super.sizeDidChange();

        // FIXME: <https://webkit.org/b/152269> Web Inspector: Convert DetailsSection classes to use View
        this._attributesDataGridRow.sizeDidChange();
    }

    // Private

    _accessibilitySupported()
    {
        return window.DOMAgent && DOMAgent.getAccessibilityPropertiesForNode;
    }

    _refreshIdentity()
    {
        const domNode = this.domNode;
        this._identityNodeTypeRow.value = this._nodeTypeDisplayName();
        this._identityNodeNameRow.value = domNode.nodeNameInCorrectCase();
        this._identityNodeValueRow.value = domNode.nodeValue();
        this._identityNodeContentSecurityPolicyHashRow.value = domNode.contentSecurityPolicyHash();
    }

    _refreshAttributes()
    {
        let domNode = this.domNode;
        if (!domNode || !domNode.hasAttributes()) {
            // Remove the DataGrid to show the placeholder text.
            this._attributesDataGridRow.dataGrid = null;
            return;
        }

        let dataGrid = this._attributesDataGridRow.dataGrid;
        if (!dataGrid) {
            const columns = {
                name: {title: WebInspector.UIString("Name"), width: "30%"},
                value: {title: WebInspector.UIString("Value")},
            };
            dataGrid = this._attributesDataGridRow.dataGrid = new WebInspector.DataGrid(columns);
        }

        dataGrid.removeChildren();

        let attributes = domNode.attributes();
        attributes.sort((a, b) => a.name.extendedLocaleCompare(b.name));
        for (let attribute of attributes) {
            let dataGridNode = new WebInspector.EditableDataGridNode(attribute);
            dataGridNode.addEventListener(WebInspector.EditableDataGridNode.Event.ValueChanged, this._attributeNodeValueChanged, this);
            dataGrid.appendChild(dataGridNode);
        }

        dataGrid.updateLayoutIfNeeded();
    }

    _refreshProperties()
    {
        if (this._nodeRemoteObject) {
            this._nodeRemoteObject.release();
            this._nodeRemoteObject = null;
        }

        let domNode = this.domNode;
        RuntimeAgent.releaseObjectGroup(WebInspector.DOMNodeDetailsSidebarPanel.PropertiesObjectGroupName);
        WebInspector.RemoteObject.resolveNode(domNode, WebInspector.DOMNodeDetailsSidebarPanel.PropertiesObjectGroupName, nodeResolved.bind(this));

        function nodeResolved(object)
        {
            if (!object)
                return;

            // Bail if the DOM node changed while we were waiting for the async response.
            if (this.domNode !== domNode)
                return;

            this._nodeRemoteObject = object;

            function inspectedPage_node_collectPrototypes()
            {
                // This builds an object with numeric properties. This is easier than dealing with arrays
                // with the way RemoteObject works. Start at 1 since we use parseInt later and parseInt
                // returns 0 for non-numeric strings make it ambiguous.
                var prototype = this;
                var result = [];
                var counter = 1;

                while (prototype) {
                    result[counter++] = prototype;
                    prototype = prototype.__proto__;
                }

                return result;
            }

            const args = undefined;
            const generatePreview = false;
            object.callFunction(inspectedPage_node_collectPrototypes, args, generatePreview, nodePrototypesReady.bind(this));
        }

        function nodePrototypesReady(error, object, wasThrown)
        {
            if (error || wasThrown || !object)
                return;

            // Bail if the DOM node changed while we were waiting for the async response.
            if (this.domNode !== domNode)
                return;

            object.deprecatedGetOwnProperties(fillSection.bind(this));
        }

        function fillSection(prototypes)
        {
            if (!prototypes)
                return;

            // Bail if the DOM node changed while we were waiting for the async response.
            if (this.domNode !== domNode)
                return;

            let element = this._propertiesRow.element;
            element.removeChildren();

            let propertyPath = new WebInspector.PropertyPath(this._nodeRemoteObject, "node");

            let initialSection = true;
            for (let i = 0; i < prototypes.length; ++i) {
                // The only values we care about are numeric, as assigned in collectPrototypes.
                if (!parseInt(prototypes[i].name, 10))
                    continue;

                let prototype = prototypes[i].value;
                let prototypeName = prototype.description;
                let title = prototypeName;
                if (/Prototype$/.test(title)) {
                    prototypeName = prototypeName.replace(/Prototype$/, "");
                    title = prototypeName + WebInspector.UIString(" (Prototype)");
                } else if (title === "Object")
                    title = title + WebInspector.UIString(" (Prototype)");

                let mode = initialSection ? WebInspector.ObjectTreeView.Mode.Properties : WebInspector.ObjectTreeView.Mode.PureAPI;
                let objectTree = new WebInspector.ObjectTreeView(prototype, mode, propertyPath);
                objectTree.showOnlyProperties();
                objectTree.setPrototypeNameOverride(prototypeName);

                let detailsSection = new WebInspector.DetailsSection(prototype.description.hash + "-prototype-properties", title, null, null, true);
                detailsSection.groups[0].rows = [new WebInspector.ObjectPropertiesDetailSectionRow(objectTree, detailsSection)];

                element.appendChild(detailsSection.element);

                initialSection = false;
            }
        }
    }

    _refreshEventListeners()
    {
        var domNode = this.domNode;
        if (!domNode)
            return;

        function createEventListenerSection(title, eventListeners, options = {}) {
            let groups = eventListeners.map((eventListener) => new WebInspector.EventListenerSectionGroup(eventListener, options));

            const optionsElement = null;
            const defaultCollapsedSettingValue = true;
            let section = new WebInspector.DetailsSection(`${title}-event-listener-section`, title, groups, optionsElement, defaultCollapsedSettingValue);
            section.element.classList.add("event-listener-section");
            return section;
        }

        function generateGroupsByEvent(eventListeners) {
            let eventListenerTypes = new Map;
            for (let eventListener of eventListeners) {
                eventListener.node = WebInspector.domTreeManager.nodeForId(eventListener.nodeId);

                let eventListenersForType = eventListenerTypes.get(eventListener.type);
                if (!eventListenersForType)
                    eventListenerTypes.set(eventListener.type, eventListenersForType = []);

                eventListenersForType.push(eventListener);
            }

            let rows = [];

            let types = Array.from(eventListenerTypes.keys());
            types.sort();
            for (let type of types)
                rows.push(createEventListenerSection(type, eventListenerTypes.get(type), {hideType: true}));

            return rows;
        }

        function generateGroupsByNode(eventListeners) {
            let eventListenerNodes = new Map;
            for (let eventListener of eventListeners) {
                eventListener.node = WebInspector.domTreeManager.nodeForId(eventListener.nodeId);

                let eventListenersForNode = eventListenerNodes.get(eventListener.node);
                if (!eventListenersForNode)
                    eventListenerNodes.set(eventListener.node, eventListenersForNode = []);

                eventListenersForNode.push(eventListener);
            }

            let rows = [];

            let currentNode = domNode;
            do {
                let eventListenersForNode = eventListenerNodes.get(currentNode);
                if (!eventListenersForNode)
                    continue;

                eventListenersForNode.sort((a, b) => a.type.toLowerCase().extendedLocaleCompare(b.type.toLowerCase()));

                rows.push(createEventListenerSection(currentNode.displayName, eventListenersForNode, {hideNode: true}));
            } while (currentNode = currentNode.parentNode);

            return rows;
        }

        function eventListenersCallback(error, eventListeners)
        {
            if (error)
                return;

            // Bail if the DOM node changed while we were waiting for the async response.
            if (this.domNode !== domNode)
                return;

            if (!eventListeners.length) {
                var emptyRow = new WebInspector.DetailsSectionRow(WebInspector.UIString("No Event Listeners"));
                emptyRow.showEmptyMessage();
                this._eventListenersSectionGroup.rows = [emptyRow];
                return;
            }

            switch (this._eventListenerGroupingMethodSetting.value) {
            case WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod.Event:
                this._eventListenersSectionGroup.rows = generateGroupsByEvent.call(this, eventListeners);
                break;

            case WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod.Node:
                this._eventListenersSectionGroup.rows = generateGroupsByNode.call(this, eventListeners);
                break;
            }
        }

        domNode.getEventListeners(eventListenersCallback.bind(this));
    }

    _refreshAccessibility()
    {
        if (!this._accessibilitySupported())
            return;

        var domNode = this.domNode;
        if (!domNode)
            return;

        var properties = {};

        function booleanValueToLocalizedStringIfTrue(property) {
            if (properties[property])
                return WebInspector.UIString("Yes");
            return "";
        }

        function booleanValueToLocalizedStringIfPropertyDefined(property) {
            if (properties[property] !== undefined) {
                if (properties[property])
                    return WebInspector.UIString("Yes");
                else
                    return WebInspector.UIString("No");
            }
            return "";
        }

        function linkForNodeId(nodeId) {
            var link = null;
            if (nodeId !== undefined && typeof nodeId === "number") {
                var node = WebInspector.domTreeManager.nodeForId(nodeId);
                if (node)
                    link = WebInspector.linkifyAccessibilityNodeReference(node);
            }
            return link;
        }

        function linkListForNodeIds(nodeIds) {
            if (!nodeIds)
                return null;

            const itemsToShow = 5;
            let hasLinks = false;
            let listItemCount = 0;
            let container = document.createElement("div");
            container.classList.add("list-container");
            let linkList = container.createChild("ul", "node-link-list");
            let initiallyHiddenItems = [];
            for (let nodeId of nodeIds) {
                let node = WebInspector.domTreeManager.nodeForId(nodeId);
                if (!node)
                    continue;
                let link = WebInspector.linkifyAccessibilityNodeReference(node);
                hasLinks = true;
                let li = linkList.createChild("li");
                li.appendChild(link);
                if (listItemCount >= itemsToShow) {
                    li.hidden = true;
                    initiallyHiddenItems.push(li);
                }
                listItemCount++;
            }
            container.appendChild(linkList);
            if (listItemCount > itemsToShow) {
                let moreNodesButton = container.createChild("button", "expand-list-button");
                moreNodesButton.textContent = WebInspector.UIString("%d More\u2026").format(listItemCount - itemsToShow);
                moreNodesButton.addEventListener("click", () => {
                    initiallyHiddenItems.forEach((element) => { element.hidden = false; });
                    moreNodesButton.remove();
                });
            }
            if (hasLinks)
                return container;

            return null;
        }

        function accessibilityPropertiesCallback(accessibilityProperties) {
            if (this.domNode !== domNode)
                return;

            // Make sure the current set of properties is available in the closure scope for the helper functions.
            properties = accessibilityProperties;

            if (accessibilityProperties && accessibilityProperties.exists) {

                var activeDescendantLink = linkForNodeId(accessibilityProperties.activeDescendantNodeId);
                var busy = booleanValueToLocalizedStringIfPropertyDefined("busy");

                var checked = "";
                if (accessibilityProperties.checked !== undefined) {
                    if (accessibilityProperties.checked === DOMAgent.AccessibilityPropertiesChecked.True)
                        checked = WebInspector.UIString("Yes");
                    else if (accessibilityProperties.checked === DOMAgent.AccessibilityPropertiesChecked.Mixed)
                        checked = WebInspector.UIString("Mixed");
                    else // DOMAgent.AccessibilityPropertiesChecked.False
                        checked = WebInspector.UIString("No");
                }

                // Accessibility tree children are not a 1:1 mapping with DOM tree children.
                var childNodeLinkList = linkListForNodeIds(accessibilityProperties.childNodeIds);
                var controlledNodeLinkList = linkListForNodeIds(accessibilityProperties.controlledNodeIds);

                var current = "";
                switch (accessibilityProperties.current) {
                case DOMAgent.AccessibilityPropertiesCurrent.True:
                    current = WebInspector.UIString("True");
                    break;
                case DOMAgent.AccessibilityPropertiesCurrent.Page:
                    current = WebInspector.UIString("Page");
                    break;
                case DOMAgent.AccessibilityPropertiesCurrent.Location:
                    current = WebInspector.UIString("Location");
                    break;
                case DOMAgent.AccessibilityPropertiesCurrent.Step:
                    current = WebInspector.UIString("Step");
                    break;
                case DOMAgent.AccessibilityPropertiesCurrent.Date:
                    current = WebInspector.UIString("Date");
                    break;
                case DOMAgent.AccessibilityPropertiesCurrent.Time:
                    current = WebInspector.UIString("Time");
                    break;
                default:
                    current = "";
                }

                var disabled = booleanValueToLocalizedStringIfTrue("disabled");
                var expanded = booleanValueToLocalizedStringIfPropertyDefined("expanded");
                var flowedNodeLinkList = linkListForNodeIds(accessibilityProperties.flowedNodeIds);
                var focused = booleanValueToLocalizedStringIfPropertyDefined("focused");

                var ignored = "";
                if (accessibilityProperties.ignored) {
                    ignored = WebInspector.UIString("Yes");
                    if (accessibilityProperties.hidden)
                        ignored = WebInspector.UIString("%s (hidden)").format(ignored);
                    else if (accessibilityProperties.ignoredByDefault)
                        ignored = WebInspector.UIString("%s (default)").format(ignored);
                }

                var invalid = "";
                if (accessibilityProperties.invalid === DOMAgent.AccessibilityPropertiesInvalid.True)
                    invalid = WebInspector.UIString("Yes");
                else if (accessibilityProperties.invalid === DOMAgent.AccessibilityPropertiesInvalid.Grammar)
                    invalid = WebInspector.UIString("Grammar");
                else if (accessibilityProperties.invalid === DOMAgent.AccessibilityPropertiesInvalid.Spelling)
                    invalid = WebInspector.UIString("Spelling");

                var label = accessibilityProperties.label;

                var liveRegionStatus = "";
                var liveRegionStatusNode = null;
                var liveRegionStatusToken = accessibilityProperties.liveRegionStatus;
                switch (liveRegionStatusToken) {
                case DOMAgent.AccessibilityPropertiesLiveRegionStatus.Assertive:
                    liveRegionStatus = WebInspector.UIString("Assertive");
                    break;
                case DOMAgent.AccessibilityPropertiesLiveRegionStatus.Polite:
                    liveRegionStatus = WebInspector.UIString("Polite");
                    break;
                default:
                    liveRegionStatus = "";
                }
                if (liveRegionStatus) {
                    var liveRegionRelevant = accessibilityProperties.liveRegionRelevant;
                    // Append @aria-relevant values. E.g. "Live: Assertive (Additions, Text)".
                    if (liveRegionRelevant && liveRegionRelevant.length) {
                        // @aria-relevant="all" is exposed as ["additions","removals","text"], in order.
                        // This order is controlled in WebCore and expected in WebInspectorUI.
                        if (liveRegionRelevant.length === 3
                            && liveRegionRelevant[0] === DOMAgent.LiveRegionRelevant.Additions
                            && liveRegionRelevant[1] === DOMAgent.LiveRegionRelevant.Removals
                            && liveRegionRelevant[2] === DOMAgent.LiveRegionRelevant.Text)
                            liveRegionRelevant = [WebInspector.UIString("All Changes")];
                        else {
                            // Reassign localized strings in place: ["additions","text"] becomes ["Additions","Text"].
                            liveRegionRelevant = liveRegionRelevant.map(function(value) {
                                switch (value) {
                                case DOMAgent.LiveRegionRelevant.Additions:
                                    return WebInspector.UIString("Additions");
                                case DOMAgent.LiveRegionRelevant.Removals:
                                    return WebInspector.UIString("Removals");
                                case DOMAgent.LiveRegionRelevant.Text:
                                    return WebInspector.UIString("Text");
                                default: // If WebCore sends a new unhandled value, display as a String.
                                    return "\"" + value + "\"";
                                }
                            });
                        }
                        liveRegionStatus += " (" + liveRegionRelevant.join(", ") + ")";
                    }
                    // Clarify @aria-atomic if necessary.
                    if (accessibilityProperties.liveRegionAtomic) {
                        liveRegionStatusNode = document.createElement("div");
                        liveRegionStatusNode.className = "value-with-clarification";
                        liveRegionStatusNode.setAttribute("role", "text");
                        liveRegionStatusNode.append(liveRegionStatus);
                        var clarificationNode = document.createElement("div");
                        clarificationNode.className = "clarification";
                        clarificationNode.append(WebInspector.UIString("Region announced in its entirety."));
                        liveRegionStatusNode.appendChild(clarificationNode);
                    }
                }

                var mouseEventNodeId = accessibilityProperties.mouseEventNodeId;
                var mouseEventTextValue = "";
                var mouseEventNodeLink = null;
                if (mouseEventNodeId) {
                    if (mouseEventNodeId === accessibilityProperties.nodeId)
                        mouseEventTextValue = WebInspector.UIString("Yes");
                    else
                        mouseEventNodeLink = linkForNodeId(mouseEventNodeId);
                }

                var ownedNodeLinkList = linkListForNodeIds(accessibilityProperties.ownedNodeIds);

                // Accessibility tree parent is not a 1:1 mapping with the DOM tree parent.
                var parentNodeLink = linkForNodeId(accessibilityProperties.parentNodeId);

                var pressed = booleanValueToLocalizedStringIfPropertyDefined("pressed");
                var readonly = booleanValueToLocalizedStringIfTrue("readonly");
                var required = booleanValueToLocalizedStringIfPropertyDefined("required");

                var role = accessibilityProperties.role;
                let hasPopup = accessibilityProperties.isPopupButton;
                let roleType = null;
                let buttonType = null;
                let buttonTypePopupString = WebInspector.UIString("popup");
                let buttonTypeToggleString = WebInspector.UIString("toggle");
                let buttonTypePopupToggleString = WebInspector.UIString("popup, toggle");

                if (role === "" || role === "unknown")
                    role = WebInspector.UIString("No matching ARIA role");
                else if (role) {
                    if (role === "button") {
                        if (pressed)
                            buttonType = buttonTypeToggleString;

                        // In cases where an element is a toggle button, but it also has
                        // aria-haspopup, we concatenate the button types. If it is just
                        // a popup button, we only include "popup".
                        if (hasPopup)
                            buttonType = buttonType ? buttonTypePopupToggleString : buttonTypePopupString;
                    }

                    if (!domNode.getAttribute("role"))
                        roleType = WebInspector.UIString("default");
                    else if (buttonType || domNode.getAttribute("role") !== role)
                        roleType = WebInspector.UIString("computed");

                    if (buttonType && roleType)
                        role = WebInspector.UIString("%s (%s, %s)").format(role, buttonType, roleType);
                    else if (roleType || buttonType) {
                        let extraInfo = roleType || buttonType;
                        role = WebInspector.UIString("%s (%s)").format(role, extraInfo);
                    }
                }

                var selected = booleanValueToLocalizedStringIfTrue("selected");
                var selectedChildNodeLinkList = linkListForNodeIds(accessibilityProperties.selectedChildNodeIds);

                var headingLevel = accessibilityProperties.headingLevel;
                var hierarchyLevel = accessibilityProperties.hierarchyLevel;
                // Assign all the properties to their respective views.
                this._accessibilityNodeActiveDescendantRow.value = activeDescendantLink || "";
                this._accessibilityNodeBusyRow.value = busy;
                this._accessibilityNodeCheckedRow.value = checked;
                this._accessibilityNodeChildrenRow.value = childNodeLinkList || "";
                this._accessibilityNodeControlsRow.value = controlledNodeLinkList || "";
                this._accessibilityNodeCurrentRow.value = current;
                this._accessibilityNodeDisabledRow.value = disabled;
                this._accessibilityNodeExpandedRow.value = expanded;
                this._accessibilityNodeFlowsRow.value = flowedNodeLinkList || "";
                this._accessibilityNodeFocusedRow.value = focused;
                this._accessibilityNodeHeadingLevelRow.value = headingLevel || "";
                this._accessibilityNodehierarchyLevelRow.value = hierarchyLevel || "";
                this._accessibilityNodeIgnoredRow.value = ignored;
                this._accessibilityNodeInvalidRow.value = invalid;
                this._accessibilityNodeLabelRow.value = label;
                this._accessibilityNodeLiveRegionStatusRow.value = liveRegionStatusNode || liveRegionStatus;

                // Row label changes based on whether the value is a delegate node link.
                this._accessibilityNodeMouseEventRow.label = mouseEventNodeLink ? WebInspector.UIString("Click Listener") : WebInspector.UIString("Clickable");
                this._accessibilityNodeMouseEventRow.value = mouseEventNodeLink || mouseEventTextValue;

                this._accessibilityNodeOwnsRow.value = ownedNodeLinkList || "";
                this._accessibilityNodeParentRow.value = parentNodeLink || "";
                this._accessibilityNodePressedRow.value = pressed;
                this._accessibilityNodeReadonlyRow.value = readonly;
                this._accessibilityNodeRequiredRow.value = required;
                this._accessibilityNodeRoleRow.value = role;
                this._accessibilityNodeSelectedRow.value = selected;

                this._accessibilityNodeSelectedChildrenRow.label = WebInspector.UIString("Selected Items");
                this._accessibilityNodeSelectedChildrenRow.value = selectedChildNodeLinkList || "";
                if (selectedChildNodeLinkList && accessibilityProperties.selectedChildNodeIds.length === 1)
                    this._accessibilityNodeSelectedChildrenRow.label = WebInspector.UIString("Selected Item");

                // Display order, not alphabetical as above.
                this._accessibilityGroup.rows = [
                    // Global properties for all elements.
                    this._accessibilityNodeIgnoredRow,
                    this._accessibilityNodeRoleRow,
                    this._accessibilityNodeLabelRow,
                    this._accessibilityNodeParentRow,
                    this._accessibilityNodeActiveDescendantRow,
                    this._accessibilityNodeSelectedChildrenRow,
                    this._accessibilityNodeChildrenRow,
                    this._accessibilityNodeOwnsRow,
                    this._accessibilityNodeControlsRow,
                    this._accessibilityNodeFlowsRow,
                    this._accessibilityNodeMouseEventRow,
                    this._accessibilityNodeFocusedRow,
                    this._accessibilityNodeHeadingLevelRow,
                    this._accessibilityNodehierarchyLevelRow,
                    this._accessibilityNodeBusyRow,
                    this._accessibilityNodeLiveRegionStatusRow,
                    this._accessibilityNodeCurrentRow,

                    // Properties exposed for all input-type elements.
                    this._accessibilityNodeDisabledRow,
                    this._accessibilityNodeInvalidRow,
                    this._accessibilityNodeRequiredRow,

                    // Role-specific properties.
                    this._accessibilityNodeCheckedRow,
                    this._accessibilityNodeExpandedRow,
                    this._accessibilityNodePressedRow,
                    this._accessibilityNodeReadonlyRow,
                    this._accessibilityNodeSelectedRow
                ];

                this._accessibilityEmptyRow.hideEmptyMessage();

            } else {
                this._accessibilityGroup.rows = [this._accessibilityEmptyRow];
                this._accessibilityEmptyRow.showEmptyMessage();
            }
        }

        domNode.accessibilityProperties(accessibilityPropertiesCallback.bind(this));
    }

    _attributesChanged(event)
    {
        if (event.data.node !== this.domNode)
            return;
        this._refreshAttributes();
        this._refreshAccessibility();
    }

    _attributeNodeValueChanged(event)
    {
        let change = event.data;
        let data = event.target.data;

        if (change.columnIdentifier === "name") {
            this.domNode.removeAttribute(data[change.columnIdentifier], (error) => {
                this.domNode.setAttribute(change.value, `${change.value}="${data.value}"`);
            });
        } else if (change.columnIdentifier === "value")
            this.domNode.setAttributeValue(data.name, change.value);
    }

    _characterDataModified(event)
    {
        if (event.data.node !== this.domNode)
            return;
        this._identityNodeValueRow.value = this.domNode.nodeValue();
    }

    _customElementStateChanged(event)
    {
        if (event.data.node !== this.domNode)
            return;
        this._refreshIdentity();
    }

    _nodeTypeDisplayName()
    {
        switch (this.domNode.nodeType()) {
        case Node.ELEMENT_NODE: {
            const nodeName = WebInspector.UIString("Element");
            const state = this._customElementState();
            return state === null ? nodeName : `${nodeName} (${state})`;
        }
        case Node.TEXT_NODE:
            return WebInspector.UIString("Text Node");
        case Node.COMMENT_NODE:
            return WebInspector.UIString("Comment");
        case Node.DOCUMENT_NODE:
            return WebInspector.UIString("Document");
        case Node.DOCUMENT_TYPE_NODE:
            return WebInspector.UIString("Document Type");
        case Node.DOCUMENT_FRAGMENT_NODE:
            return WebInspector.UIString("Document Fragment");
        case Node.CDATA_SECTION_NODE:
            return WebInspector.UIString("Character Data");
        case Node.PROCESSING_INSTRUCTION_NODE:
            return WebInspector.UIString("Processing Instruction");
        default:
            console.error("Unknown DOM node type: ", this.domNode.nodeType());
            return this.domNode.nodeType();
        }
    }

    _customElementState()
    {
        const state = this.domNode.customElementState();
        switch (state) {
        case WebInspector.DOMNode.CustomElementState.Builtin:
            return null;
        case WebInspector.DOMNode.CustomElementState.Custom:
            return WebInspector.UIString("Custom");
        case WebInspector.DOMNode.CustomElementState.Waiting:
            return WebInspector.UIString("Waiting to be upgraded");
        case WebInspector.DOMNode.CustomElementState.Failed:
            return WebInspector.UIString("Failed to upgrade");
        }
        console.error("Unknown DOM custom element state: ", state);
        return null;
    }
};

WebInspector.DOMNodeDetailsSidebarPanel.EventListenerGroupingMethod = {
    Event: "event",
    Node: "node",
};

WebInspector.DOMNodeDetailsSidebarPanel.PropertiesObjectGroupName = "dom-node-details-sidebar-properties-object-group";
