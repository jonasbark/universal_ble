import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:universal_ble/src/universal_ble.g.dart';
import 'package:universal_ble/universal_ble.dart';

/// Native errors reach Dart through [UniversalBle.nativeLogHandler] in every
/// build mode, unlike [UniversalBle.setLogLevel] which is debug-only.
void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  const channel =
      'dev.flutter.pigeon.universal_ble.UniversalBleNativeLogCallback.onNativeLog';

  Future<void> sendFromNative(BleLogLevel level, String message) async {
    final data = UniversalBleNativeLogCallback.pigeonChannelCodec
        .encodeMessage(<Object?>[level, message]);
    await TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger
        .handlePlatformMessage(channel, data, (_) {});
  }

  tearDown(() => UniversalBle.nativeLogHandler = null);

  test('delivers native errors and warnings to the handler', () async {
    final received = <(BleLogLevel, String)>[];
    UniversalBle.nativeLogHandler =
        (level, message) => received.add((level, message));

    await sendFromNative(BleLogLevel.error,
        'PERIPHERAL_READ_REQ response hresult_error hr=-2147483629');
    await sendFromNative(BleLogLevel.warning,
        'DisposeConnection: failed to close Bluetooth device');

    expect(received, [
      (
        BleLogLevel.error,
        'PERIPHERAL_READ_REQ response hresult_error hr=-2147483629'
      ),
      (
        BleLogLevel.warning,
        'DisposeConnection: failed to close Bluetooth device'
      ),
    ]);
  });

  test('a null handler stops delivery', () async {
    final received = <String>[];
    UniversalBle.nativeLogHandler = (_, message) => received.add(message);
    UniversalBle.nativeLogHandler = null;

    await sendFromNative(BleLogLevel.error, 'dropped');

    expect(received, isEmpty);
  });
}
