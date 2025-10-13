import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'device_screen.dart';
import '../utils/snackbar.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({super.key});

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  bool _isScanning = false;
  late StreamSubscription<List<ScanResult>> _scanResultsSubscription;
  late StreamSubscription<bool> _isScanningSubscription;

  @override
  void initState() {
    super.initState();

    _scanResultsSubscription = FlutterBluePlus.scanResults.listen(
      (results) {
        if (results.isNotEmpty) {
          FlutterBluePlus.stopScan();
          BluetoothDevice device = results.first.device;
          MaterialPageRoute route = MaterialPageRoute(
            builder: (context) => DeviceScreen(device: device),
            settings: RouteSettings(name: '/DeviceScreen'),
          );
          Navigator.of(context).push(route);
        }
      },
      onError: (e) {
        Snackbar.show(ABC.b, prettyException("Scan Error:", e), success: false);
      },
    );

    _isScanningSubscription = FlutterBluePlus.isScanning.listen((state) {
      if (mounted) {
        setState(() => _isScanning = state);
      }
    });
  }

  @override
  void dispose() {
    _scanResultsSubscription.cancel();
    _isScanningSubscription.cancel();
    super.dispose();
  }

  Future onScanPressed() async {
    if (_isScanning) {
      return;
    }

    try {
      await FlutterBluePlus.startScan(
        timeout: const Duration(seconds: 15),
        withNames: ["Mastermind"],
        webOptionalServices: [
          Guid("180a"), // device info
          Guid("1800"), // generic access
          Guid("00001523-2929-efde-1523-785feabcd123"), // mastermind service
        ],
      );
    } catch (e, backtrace) {
      Snackbar.show(ABC.b, prettyException("Start Scan Error:", e), success: false);
      print(e);
      print("backtrace: $backtrace");
    }
    if (mounted) {
      setState(() {});
    }
  }

  Widget buildBluetoothSearchingIcon(BuildContext context) {
    return const Icon(Icons.bluetooth_searching, size: 200.0, color: Colors.white54);
  }

  Widget buildTitle(BuildContext context) {
    String state = _isScanning ? "Searching Mastermind device" : "Click to start Mastermind search";
    return Text(state, style: Theme.of(context).primaryTextTheme.titleSmall?.copyWith(color: Colors.white));
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyA,
      child: Scaffold(
        backgroundColor: Colors.lightBlue,
        body: GestureDetector(
          behavior: HitTestBehavior.opaque,
          onTap: () => onScanPressed(),
          child: Center(
            child: Column(mainAxisSize: MainAxisSize.min, children: <Widget>[buildBluetoothSearchingIcon(context), buildTitle(context)]),
          ),
        ),
      ),
    );
  }
}
