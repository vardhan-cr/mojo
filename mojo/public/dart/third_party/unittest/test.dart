import 'dart:async';

main() async {
  try {
    try {
      await new Future.error('error');
    } catch (error) {
      print("caught once");
      throw 'error';
    }
  } catch (error) {
    print("caught twice");
    throw 'error';
  }
}
