/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);

loader.lazyImporter(this, "Heritage",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "AbstractTreeItem",
  "resource:///modules/devtools/AbstractTreeItem.jsm");

const URL_LABEL_TOOLTIP = L10N.getStr("table.url.tooltiptext");
const ZOOM_BUTTON_TOOLTIP = L10N.getStr("table.zoom.tooltiptext");
const CALL_TREE_INDENTATION = 16; // px
const CALL_TREE_AUTO_EXPAND = 3; // depth

exports.CallView = CallView;

/**
 * An item in a call tree view, which looks like this:
 *
 *   Time (ms)  |   Cost   | Calls | Function
 * ============================================================================
 *     1,000.00 |  100.00% |       | ▼ (root)
 *       500.12 |   50.01% |   300 |   ▼ foo                          Categ. 1
 *       300.34 |   30.03% |  1500 |     ▼ bar                        Categ. 2
 *        10.56 |    0.01% |    42 |       ▶ call_with_children       Categ. 3
 *        90.78 |    0.09% |    25 |         call_without_children    Categ. 4
 *
 * Every instance of a `CallView` represents a row in the call tree. The same
 * parent node is used for all rows.
 *
 * @param CallView caller
 *        The CallView considered the "caller" frame. This instance will be
 *        represent the "callee". Should be null for root nodes.
 * @param ThreadNode | FrameNode frame
 *        Details about this function, like { duration, invocation, calls } etc.
 * @param number level
 *        The indentation level in the call tree. The root node is at level 0.
 */
function CallView({ caller, frame, level }) {
  AbstractTreeItem.call(this, { parent: caller, level: level });

  this.autoExpandDepth = caller ? caller.autoExpandDepth : CALL_TREE_AUTO_EXPAND;
  this.frame = frame;

  this._onUrlClick = this._onUrlClick.bind(this);
  this._onZoomClick = this._onZoomClick.bind(this);
};

CallView.prototype = Heritage.extend(AbstractTreeItem.prototype, {
  /**
   * Creates the view for this tree node.
   * @param nsIDOMNode document
   * @param nsIDOMNode arrowNode
   * @return nsIDOMNode
   */
  _displaySelf: function(document, arrowNode) {
    this.document = document;

    let frameInfo = this.frame.getInfo();
    let framePercentage = this.frame.duration / this.root.frame.duration * 100;

    let durationCell = this._createTimeCell(this.frame.duration);
    let percentageCell = this._createExecutionCell(framePercentage);
    let invocationsCell = this._createInvocationsCell(this.frame.invocations);
    let functionCell = this._createFunctionCell(arrowNode, frameInfo, this.level);

    let targetNode = document.createElement("hbox");
    targetNode.className = "call-tree-item";
    targetNode.setAttribute("origin", frameInfo.isContent ? "content" : "chrome");
    targetNode.setAttribute("category", frameInfo.categoryData.abbrev || "");
    targetNode.setAttribute("tooltiptext", this.frame.location || "");

    let isRoot = frameInfo.nodeType == "Thread";
    if (isRoot) {
      functionCell.querySelector(".call-tree-zoom").hidden = true;
      functionCell.querySelector(".call-tree-category").hidden = true;
    }

    targetNode.appendChild(durationCell);
    targetNode.appendChild(percentageCell);
    targetNode.appendChild(invocationsCell);
    targetNode.appendChild(functionCell);

    return targetNode;
  },

  /**
   * Populates this node in the call tree with the corresponding "callees".
   * These are defined in the `frame` data source for this call view.
   * @param array:AbstractTreeItem children
   */
  _populateSelf: function(children) {
    let newLevel = this.level + 1;

    for (let [, newFrame] of _Iterator(this.frame.calls)) {
      children.push(new CallView({
        caller: this,
        frame: newFrame,
        level: newLevel
      }));
    }

    // Sort the "callees" asc. by duration, before inserting them in the tree.
    children.sort((a, b) => a.frame.duration < b.frame.duration ? 1 : -1);
  },

  /**
   * Functions creating each cell in this call view.
   * Invoked by `_displaySelf`.
   */
  _createTimeCell: function(duration) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "duration");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(duration, 2));
    return cell;
  },
  _createExecutionCell: function(percentage) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "percentage");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", L10N.numberWithDecimals(percentage, 2) + "%");
    return cell;
  },
  _createInvocationsCell: function(count) {
    let cell = this.document.createElement("label");
    cell.className = "plain call-tree-cell";
    cell.setAttribute("type", "invocations");
    cell.setAttribute("crop", "end");
    cell.setAttribute("value", count || "");
    return cell;
  },
  _createFunctionCell: function(arrowNode, frameInfo, frameLevel) {
    let cell = this.document.createElement("hbox");
    cell.className = "call-tree-cell";
    cell.style.MozMarginStart = (frameLevel * CALL_TREE_INDENTATION) + "px";
    cell.setAttribute("type", "function");
    cell.appendChild(arrowNode);

    let nameNode = this.document.createElement("label");
    nameNode.className = "plain call-tree-name";
    nameNode.setAttribute("flex", "1");
    nameNode.setAttribute("crop", "end");
    nameNode.setAttribute("value", frameInfo.functionName || "");
    cell.appendChild(nameNode);

    let urlNode = this.document.createElement("label");
    urlNode.className = "plain call-tree-url";
    urlNode.setAttribute("flex", "1");
    urlNode.setAttribute("crop", "end");
    urlNode.setAttribute("value", frameInfo.fileName || "");
    urlNode.setAttribute("tooltiptext", URL_LABEL_TOOLTIP + " → " + frameInfo.url);
    urlNode.addEventListener("mousedown", this._onUrlClick);
    cell.appendChild(urlNode);

    let lineNode = this.document.createElement("label");
    lineNode.className = "plain call-tree-line";
    lineNode.setAttribute("value", frameInfo.line ? ":" + frameInfo.line : "");
    cell.appendChild(lineNode);

    let hostNode = this.document.createElement("label");
    hostNode.className = "plain call-tree-host";
    hostNode.setAttribute("value", frameInfo.hostName || "");
    cell.appendChild(hostNode);

    let zoomNode = this.document.createElement("button");
    zoomNode.className = "plain call-tree-zoom";
    zoomNode.setAttribute("tooltiptext", ZOOM_BUTTON_TOOLTIP);
    zoomNode.addEventListener("mousedown", this._onZoomClick);
    cell.appendChild(zoomNode);

    let spacerNode = this.document.createElement("spacer");
    spacerNode.setAttribute("flex", "10000");
    cell.appendChild(spacerNode);

    let categoryNode = this.document.createElement("label");
    categoryNode.className = "plain call-tree-category";
    categoryNode.style.color = frameInfo.categoryData.color;
    categoryNode.setAttribute("value", frameInfo.categoryData.label || "");
    cell.appendChild(categoryNode);

    let hasDescendants = Object.keys(this.frame.calls).length > 0;
    if (hasDescendants == false) {
      arrowNode.setAttribute("invisible", "");
    }

    return cell;
  },

  /**
   * Toggles the category information hidden or visible.
   * @param boolean visible
   */
  toggleCategories: function(visible) {
    if (!visible) {
      this.container.setAttribute("categories-hidden", "");
    } else {
      this.container.removeAttribute("categories-hidden");
    }
  },

  /**
   * Handler for the "click" event on the url node of this call view.
   */
  _onUrlClick: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.root.emit("link", this);
  },

  /**
   * Handler for the "click" event on the zoom node of this call view.
   */
  _onZoomClick: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this.root.emit("zoom", this);
  }
});
