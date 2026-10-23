import Foundation

/// FIFO of notifications CoreBluetooth could not accept yet.
///
/// `CBPeripheralManager.updateValue` returns `false` when its transmit queue is
/// full and calls `peripheralManagerIsReady(toUpdateSubscribers:)` once there is
/// room again. Notifications sent back to back (e.g. a button press immediately
/// followed by its release) regularly hit that limit, so refused items are kept
/// here and resent in order on the ready callback instead of being dropped.
///
/// Not thread-safe: use it from the peripheral manager's delegate queue.
final class PendingNotificationQueue<Item> {
  private let capacity: Int
  private var items = [Item]()

  init(capacity: Int) {
    self.capacity = capacity
  }

  var count: Int { items.count }

  /// Sends `item` right away when nothing is waiting and `send` accepts it;
  /// otherwise queues it behind the pending items so ordering is kept.
  /// Returns `false` when the queue is full and the item was dropped.
  func submit(_ item: Item, send: (Item) -> Bool) -> Bool {
    if items.isEmpty && send(item) {
      return true
    }
    guard items.count < capacity else {
      return false
    }
    items.append(item)
    return true
  }

  /// Sends pending items in order until `send` refuses one.
  func drain(send: (Item) -> Bool) {
    while let item = items.first {
      guard send(item) else { return }
      items.removeFirst()
    }
  }

  func removeAll(where shouldRemove: (Item) -> Bool) {
    items.removeAll(where: shouldRemove)
  }

  func removeAll() {
    items.removeAll()
  }
}
