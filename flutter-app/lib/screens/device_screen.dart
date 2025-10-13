import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../utils/snackbar.dart';
import '../utils/extra.dart';
import '../utils/combination.dart';

class DeviceScreen extends StatefulWidget {
  final BluetoothDevice device;

  const DeviceScreen({super.key, required this.device});

  @override
  State<DeviceScreen> createState() => _DeviceScreenState();
}

class _DeviceScreenState extends State<DeviceScreen> {
  BluetoothConnectionState _connectionState = BluetoothConnectionState.disconnected;
  BluetoothService? _mastermindService;
  BluetoothCharacteristic? _readCharacteristic;
  BluetoothCharacteristic? _writeCharacteristic;
  bool _isDiscoveringServices = false;
  bool _isConnecting = false;
  bool _isDisconnecting = false;
  Combination? code;
  List<Combination> tentatives = [];
  late StreamSubscription<BluetoothConnectionState> _connectionStateSubscription;
  late StreamSubscription<bool> _isConnectingSubscription;
  late StreamSubscription<bool> _isDisconnectingSubscription;
  late StreamSubscription<List<int>> _lastValueSubscription;

  @override
  void initState() {
    super.initState();

    _connectionStateSubscription = widget.device.connectionState.listen((state) async {
      _connectionState = state;
      if (state == BluetoothConnectionState.connected) {
        _isDiscoveringServices = true;
        try {
          // Discover and search mastermind main service
          List<BluetoothService> services = await widget.device.discoverServices();
          _mastermindService = services.firstWhere((service) => service.uuid == Guid("00001523-2929-efde-1523-785feabcd123"));
          if (_mastermindService == null) {
            throw Exception("Mastermind service not found");
          }

          // Enable notification
          _readCharacteristic = _mastermindService!.characteristics.firstWhere((char) => char.uuid == Guid("00001524-2929-efde-1523-785feabcd123"));
          if (_readCharacteristic == null) {
            throw Exception("Read characteristic not found");
          }

          _lastValueSubscription = _readCharacteristic!.lastValueStream.listen((buf) {
            // Process the value received from the characteristic
            // Parse the code (solution)
            code = Combination.fromBuffer(buf.sublist(0, 10));

            // Parse the tentatives
            tentatives.clear();
            for (int i = 0; i < buf[10]; i++) {
              tentatives.add(Combination.fromBuffer(buf.sublist(11 + i * 10, 21 + i * 10)));
            }

            // Refresh the UI
            if (mounted) {
              setState(() {});
            }
          });

          _readCharacteristic!.read();

          if (!await _readCharacteristic!.setNotifyValue(true)) {
            throw Exception("Failed to enable notification");
          }

          _writeCharacteristic = _mastermindService!.characteristics.firstWhere((char) => char.uuid == Guid("00001525-2929-efde-1523-785feabcd123"));
          if (_writeCharacteristic == null) {
            throw Exception("Write characteristic not found");
          }

          Snackbar.show(ABC.c, "Finding service and characteristics : Success", success: true);
        } catch (e, backtrace) {
          Snackbar.show(ABC.c, prettyException("Discover Services Error:", e), success: false);
          print(e);
          print("backtrace: $backtrace");
        }
        _isDiscoveringServices = false;
      } else if (state == BluetoothConnectionState.disconnected) {
        _lastValueSubscription.cancel();
        _mastermindService = null;
        _readCharacteristic = null;
        _writeCharacteristic = null;
      }
      if (mounted) {
        setState(() {});
      }
    });

    _isConnectingSubscription = widget.device.isConnecting.listen((value) {
      _isConnecting = value;
      if (mounted) {
        setState(() {});
      }
    });

    _isDisconnectingSubscription = widget.device.isDisconnecting.listen((value) {
      _isDisconnecting = value;
      if (mounted) {
        setState(() {});
      }
    });

    if (!_isConnecting) {
      widget.device.connectAndUpdateStream().catchError((e) {
        Snackbar.show(ABC.c, prettyException("Connect Error:", e), success: false);
      });
    }
  }

  @override
  void dispose() {
    _connectionStateSubscription.cancel();
    _isConnectingSubscription.cancel();
    _isDisconnectingSubscription.cancel();
    _lastValueSubscription.cancel();
    super.dispose();
  }

  bool get isConnected {
    return _connectionState == BluetoothConnectionState.connected;
  }

  void reset() {
    if (_writeCharacteristic != null) {
      _writeCharacteristic!.write([0x00]);
    }
  }

  void off() {
    if (_writeCharacteristic != null) {
      _writeCharacteristic!.write([0x01]);
    }
  }

  void setCode(Combination newCode) {
    if (_writeCharacteristic != null) {
      _writeCharacteristic!.write([0x02] + newCode.toBuffer());
    }
  }

  Future onConnectPressed() async {
    try {
      await widget.device.connectAndUpdateStream();
      Snackbar.show(ABC.c, "Connect: Success", success: true);
    } catch (e, backtrace) {
      if (e is FlutterBluePlusException && e.code == FbpErrorCode.connectionCanceled.index) {
        // ignore connections canceled by the user
      } else {
        Snackbar.show(ABC.c, prettyException("Connect Error:", e), success: false);
        print(e);
        print("backtrace: $backtrace");
      }
    }
  }

  Future onCancelPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream(queue: false);
      Snackbar.show(ABC.c, "Cancel: Success", success: true);
    } catch (e, backtrace) {
      Snackbar.show(ABC.c, prettyException("Cancel Error:", e), success: false);
      print("$e");
      print("backtrace: $backtrace");
    }
  }

  Future onDisconnectPressed() async {
    try {
      await widget.device.disconnectAndUpdateStream();
      Snackbar.show(ABC.c, "Disconnect: Success", success: true);
    } catch (e, backtrace) {
      Snackbar.show(ABC.c, prettyException("Disconnect Error:", e), success: false);
      print("$e backtrace: $backtrace");
    }
  }

  Widget buildSpinner(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(14.0),
      child: AspectRatio(
        aspectRatio: 1.0,
        child: CircularProgressIndicator(backgroundColor: Colors.transparent, color: Colors.white),
      ),
    );
  }

  Widget buildConnectButton(BuildContext context) {
    return Row(
      children: [
        if (_isConnecting || _isDisconnecting) buildSpinner(context),
        ElevatedButton(
          onPressed: _isConnecting ? onCancelPressed : (isConnected ? onDisconnectPressed : onConnectPressed),
          style: ElevatedButton.styleFrom(backgroundColor: isConnected ? Colors.redAccent : Colors.green, foregroundColor: Colors.white),
          child: Text(
            _isConnecting ? "CANCEL" : (isConnected ? "DISCONNECT" : "CONNECT"),
            style: Theme.of(context).primaryTextTheme.labelLarge?.copyWith(color: Colors.white),
          ),
        ),
      ],
    );
  }

  Widget buildTentativeItem(BuildContext context, int index) {
    Combination tentative = tentatives[index];
    return SizedBox(
      height: 45,
      child: Row(
        children: [
          Expanded(
            flex: 10,
            child: Center(
              child: Text(
                textAlign: TextAlign.center,
                "${index + 1}",
                style: TextStyle(fontSize: 25, color: Colors.black),
              ),
            ),
          ),
          Expanded(
            flex: 60,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: List.generate(4, (index) {
                return Container(
                  margin: const EdgeInsets.symmetric(horizontal: 6),
                  width: 40,
                  height: 40,
                  decoration: BoxDecoration(
                    color: tentative.slots[index],
                    border: Border.all(color: Colors.black, width: 1.5),
                    shape: BoxShape.circle,
                  ),
                );
              }),
            ),
          ),

          Expanded(
            flex: 20,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                Expanded(
                  child: Center(
                    child: Text(
                      tentative.cluesCorrect.toString(),
                      textAlign: TextAlign.center,
                      style: TextStyle(fontSize: 35, fontWeight: FontWeight.bold, color: Colors.green),
                    ),
                  ),
                ),
                Expanded(
                  child: Center(
                    child: Text(
                      tentative.cluesPresent.toString(),
                      textAlign: TextAlign.center,
                      style: TextStyle(fontSize: 35, fontWeight: FontWeight.bold, color: Colors.orange),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget buildTentativeListHeader(BuildContext context) {
    return Container(
      padding: EdgeInsets.only(left: 10.0, right: 10.0, top: 10.0),
      child: Row(
        children: [
          Expanded(
            flex: 10,
            child: Center(
              child: Text(
                "#",
                style: TextStyle(fontSize: 25, color: Colors.black, fontWeight: FontWeight.bold),
              ),
            ),
          ),
          Expanded(
            flex: 60,
            child: Center(
              child: Text(
                "Colors",
                style: TextStyle(fontSize: 25, color: Colors.black, fontWeight: FontWeight.bold),
              ),
            ),
          ),
          Expanded(
            flex: 20,
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                Icon(Icons.check_circle_rounded, color: Colors.green, size: 35),
                Icon(Icons.color_lens, color: Colors.orange, size: 35),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget buildTentativesList(BuildContext context) {
    return Column(
      children: [
        buildTentativeListHeader(context),
        ListView.builder(
          padding: EdgeInsets.all(10.0),
          shrinkWrap: true,
          reverse: true,
          itemCount: tentatives.length,
          itemBuilder: (BuildContext context, int index) {
            return Column(children: [buildTentativeItem(context, index), Divider()]);
          },
        ),
      ],
    );
  }

  Widget buildActionButtons(BuildContext context) {
    return Container(
      margin: const EdgeInsets.all(10.0),
      child: Column(
        spacing: 10,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            spacing: 20,
            children: [
              SizedBox(
                width: 150,
                child: TextButton.icon(
                  onPressed: () {
                    showCodeDialog();
                  },
                  icon: const Icon(Icons.lightbulb_rounded, size: 30),
                  label: const Text('Show Code'),
                  style: TextButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.green),
                ),
              ),
              SizedBox(
                width: 150,
                child: TextButton.icon(
                  onPressed: () {
                    showSetCodeDialog();
                  },
                  icon: const Icon(Icons.settings_rounded, size: 30),
                  label: const Text('Set Code'),
                  style: TextButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.indigo),
                ),
              ),
            ],
          ),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            spacing: 20,
            children: [
              SizedBox(
                width: 150,
                child: TextButton.icon(
                  onPressed: () {
                    reset();
                  },
                  icon: const Icon(Icons.restart_alt_rounded, size: 30),
                  label: const Text('Restart'),
                  style: TextButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.orange),
                ),
              ),
              SizedBox(
                width: 150,
                child: TextButton.icon(
                  onPressed: () {
                    off();
                  },
                  icon: const Icon(Icons.power_off_rounded, size: 30),
                  label: const Text('Power Off'),
                  style: TextButton.styleFrom(foregroundColor: Colors.white, backgroundColor: Colors.redAccent),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  void showSetCodeDialog() {
    Combination newCode = Combination.fromRandom();
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setStateDialog) {
            return AlertDialog(
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
              title: const Text('Select the new code', style: TextStyle(fontWeight: FontWeight.bold)),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: List.generate(4, (index) {
                      return GestureDetector(
                        onTap: () => {
                          setStateDialog(() {
                            int newColorIndex = (newCode.colors.indexOf(newCode.slots[index]) + 1) % newCode.colors.length;
                            newCode.slots[index] = newCode.colors[newColorIndex];
                          }),
                        },
                        child: Container(
                          margin: const EdgeInsets.symmetric(horizontal: 6),
                          width: 40,
                          height: 40,
                          decoration: BoxDecoration(
                            color: newCode.slots[index],
                            border: Border.all(color: Colors.black, width: 1.5),
                            shape: BoxShape.circle,
                          ),
                        ),
                      );
                    }),
                  ),
                  const SizedBox(height: 12),
                ],
              ),
              actions: [
                ElevatedButton(
                  onPressed: () {
                    Navigator.pop(context);
                    setCode(newCode);
                    setState(() {});
                  },
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.green, foregroundColor: Colors.white),
                  child: const Text('Confirm'),
                ),
              ],
            );
          },
        );
      },
    );
  }

  void showCodeDialog() {
    if (code == null) {
      Snackbar.show(ABC.c, "Code was not received", success: false);
      return;
    }

    showDialog(
      context: context,
      builder: (BuildContext context) {
        return StatefulBuilder(
          builder: (context, setStateDialog) {
            return AlertDialog(
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
              title: const Text('The correct code is', style: TextStyle(fontWeight: FontWeight.bold)),
              content: Column(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: List.generate(4, (index) {
                      return Container(
                        margin: const EdgeInsets.symmetric(horizontal: 6),
                        width: 40,
                        height: 40,
                        decoration: BoxDecoration(
                          color: code!.slots[index],
                          border: Border.all(color: Colors.black, width: 1.5),
                          shape: BoxShape.circle,
                        ),
                      );
                    }),
                  ),
                  const SizedBox(height: 12),
                ],
              ),
            );
          },
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return ScaffoldMessenger(
      key: Snackbar.snackBarKeyC,
      child: Scaffold(
        appBar: AppBar(
          title: Text(widget.device.platformName),
          automaticallyImplyLeading: false,
          actions: [buildConnectButton(context), const SizedBox(width: 15)],
          backgroundColor: isConnected ? Colors.green : Colors.redAccent,
          foregroundColor: Colors.white,
        ),
        body: Column(children: [buildActionButtons(context), buildTentativesList(context)]),
      ),
    );
  }
}
