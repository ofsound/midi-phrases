/**
 * Keep an element out of the tab order. svelte-dnd-action drag handles reset
 * tabIndex to 0 for keyboard reordering; this plugin is pointer-only.
 * @param {HTMLElement} node
 */
export function preventTabFocus(node) {
  const enforce = () => {
    if (node.tabIndex !== -1) {
      node.tabIndex = -1;
    }
  };

  enforce();

  const observer = new MutationObserver(enforce);
  observer.observe(node, { attributes: true, attributeFilter: ["tabindex"] });

  return {
    destroy() {
      observer.disconnect();
    },
  };
}
