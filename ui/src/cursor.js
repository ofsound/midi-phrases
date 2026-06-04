export function setActiveCursor(cursor) {
  document.body.dataset.cursor = cursor;
}

export function clearActiveCursor(cursor) {
  if (cursor === undefined || document.body.dataset.cursor === cursor) {
    delete document.body.dataset.cursor;
  }
}
