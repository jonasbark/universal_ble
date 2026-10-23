import Cocoa
import XCTest

@testable import universal_ble

class PendingNotificationQueueTests: XCTestCase {

  func testSendsImmediatelyWhenNothingIsPending() {
    let queue = PendingNotificationQueue<Int>(capacity: 4)
    var sent = [Int]()

    XCTAssertTrue(queue.submit(1) { sent.append($0); return true })

    XCTAssertEqual(sent, [1])
    XCTAssertEqual(queue.count, 0)
  }

  func testQueuesWhenTransmitQueueIsFullAndSendsOnDrain() {
    let queue = PendingNotificationQueue<Int>(capacity: 4)
    var sent = [Int]()

    XCTAssertTrue(queue.submit(1) { sent.append($0); return true })
    XCTAssertTrue(queue.submit(2) { _ in false })  // press went out, release refused

    XCTAssertEqual(sent, [1])
    XCTAssertEqual(queue.count, 1)

    queue.drain { sent.append($0); return true }

    XCTAssertEqual(sent, [1, 2])
    XCTAssertEqual(queue.count, 0)
  }

  func testKeepsOrderWhileItemsArePending() {
    let queue = PendingNotificationQueue<Int>(capacity: 4)
    var sent = [Int]()

    XCTAssertTrue(queue.submit(1) { _ in false })
    // Transmit queue has room again, but 1 must still go out before 2.
    XCTAssertTrue(queue.submit(2) { sent.append($0); return true })

    XCTAssertEqual(sent, [])
    XCTAssertEqual(queue.count, 2)

    queue.drain { sent.append($0); return true }

    XCTAssertEqual(sent, [1, 2])
  }

  func testDrainStopsAtFirstRefusalAndKeepsTheRest() {
    let queue = PendingNotificationQueue<Int>(capacity: 4)
    _ = queue.submit(1) { _ in false }
    _ = queue.submit(2) { _ in false }
    _ = queue.submit(3) { _ in false }
    var sent = [Int]()

    queue.drain { item in
      guard item == 1 else { return false }
      sent.append(item)
      return true
    }

    XCTAssertEqual(sent, [1])
    XCTAssertEqual(queue.count, 2)

    queue.drain { sent.append($0); return true }

    XCTAssertEqual(sent, [1, 2, 3])
  }

  func testRejectsWhenFull() {
    let queue = PendingNotificationQueue<Int>(capacity: 2)
    XCTAssertTrue(queue.submit(1) { _ in false })
    XCTAssertTrue(queue.submit(2) { _ in false })

    XCTAssertFalse(queue.submit(3) { _ in true })
    XCTAssertEqual(queue.count, 2)
  }

  func testRemoveAllWhereDropsMatchingItems() {
    let queue = PendingNotificationQueue<Int>(capacity: 4)
    _ = queue.submit(1) { _ in false }
    _ = queue.submit(2) { _ in false }
    _ = queue.submit(3) { _ in false }

    queue.removeAll { $0 == 2 }

    var sent = [Int]()
    queue.drain { sent.append($0); return true }
    XCTAssertEqual(sent, [1, 3])
  }
}
