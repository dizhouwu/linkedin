from automat import MethodicalMachine
from enum import Enum

class MarketDataState(Enum):
    DISCONNECTED = 1
    CONNECTING = 2
    CONNECTED = 3
    SUBSCRIBING = 4
    SUBSCRIBED = 5
    ERROR = 6

class MarketDataListener:
    _machine = MethodicalMachine()

    def __init__(self):
        self.state = MarketDataState.DISCONNECTED
        self.subscribed_symbols = set()
        self.connection_attempts = 0

    @_machine.state(initial=True)
    def disconnected(self):
        pass

    @_machine.state()
    def connecting(self):
        pass

    @_machine.state()
    def connected(self):
        pass

    @_machine.state()
    def subscribing(self):
        pass

    @_machine.state()
    def subscribed(self):
        pass

    @_machine.state()
    def error(self):
        pass

    @_machine.input()
    def connect(self):
        pass

    @_machine.input()
    def connection_success(self):
        pass

    @_machine.input()
    def connection_failure(self):
        pass

    @_machine.input()
    def subscribe(self, symbol):
        pass

    @_machine.input()
    def subscription_success(self, symbol):
        pass

    @_machine.input()
    def subscription_failure(self, symbol):
        pass

    @_machine.input()
    def disconnect(self):
        pass

    @_machine.input()
    def error_occurred(self, error_message):
        pass

    @_machine.output()
    def _start_connection(self):
        print("Starting connection...")
        self.state = MarketDataState.CONNECTING
        self.connection_attempts += 1
        # Simulate connection attempt (replace with actual logic)
        if self.connection_attempts < 3:
            self.connection_success()
        else:
            self.connection_failure()

    @_machine.output()
    def _handle_connection_success(self):
        print("Connection successful.")
        self.state = MarketDataState.CONNECTED
        self.connection_attempts = 0

    @_machine.output()
    def _handle_connection_failure(self):
        print("Connection failed.")
        self.state = MarketDataState.ERROR

    @_machine.output()
    def _start_subscription(self, symbol):
        print(f"Subscribing to {symbol}...")
        self.state = MarketDataState.SUBSCRIBING
        # Simulate subscription attempt (replace with actual logic)
        if len(self.subscribed_symbols) < 2:
            self.subscription_success(symbol)
        else:
            self.subscription_failure(symbol)

    @_machine.output()
    def _handle_subscription_success(self, symbol):
        print(f"Subscribed to {symbol}.")
        self.state = MarketDataState.SUBSCRIBED
        self.subscribed_symbols.add(symbol)

    @_machine.output()
    def _handle_subscription_failure(self, symbol):
        print(f"Subscription to {symbol} failed.")
        self.state = MarketDataState.ERROR

    @_machine.output()
    def _handle_disconnect(self):
        print("Disconnecting...")
        self.state = MarketDataState.DISCONNECTED
        self.subscribed_symbols.clear()

    @_machine.output()
    def _handle_error(self, error_message):
        print(f"Error: {error_message}")
        self.state = MarketDataState.ERROR

    disconnected.upon(connect, enter=connecting, outputs=[_start_connection])
    connecting.upon(connection_success, enter=connected, outputs=[_handle_connection_success])
    connecting.upon(connection_failure, enter=error, outputs=[_handle_connection_failure])
    connected.upon(subscribe, enter=subscribing, outputs=[_start_subscription])
    subscribing.upon(subscription_success, enter=subscribed, outputs=[_handle_subscription_success])
    subscribing.upon(subscription_failure, enter=error, outputs=[_handle_subscription_failure])
    subscribed.upon(subscribe, enter=subscribing, outputs=[_start_subscription])
    subscribed.upon(disconnect, enter=disconnected, outputs=[_handle_disconnect])
    connected.upon(disconnect, enter=disconnected, outputs=[_handle_disconnect])
    error.upon(connect, enter=connecting, outputs=[_start_connection])
    disconnected.upon(error_occurred, enter=error, outputs=[_handle_error])
    connected.upon(error_occurred, enter=error, outputs=[_handle_error])
    subscribed.upon(error_occurred, enter=error, outputs=[_handle_error])
    connecting.upon(error_occurred, enter=error, outputs=[_handle_error])
    subscribing.upon(error_occurred, enter=error, outputs=[_handle_error])
    error.upon(disconnect, enter=disconnected, outputs=[_handle_disconnect]) #add this line.

# Example usage
listener = MarketDataListener()
listener.connect()
listener.subscribe("AAPL")
listener.subscribe("GOOG")
listener.subscribe("MSFT") #Will cause a failure in the simulation
listener.disconnect()
listener.connect()
listener.subscribe("AAPL")