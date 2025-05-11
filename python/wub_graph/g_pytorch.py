from collections import defaultdict
from datetime import date
from enum import Enum

class Mode(Enum):
    CACHED = "cached"
    COMPUTED = "computed"

class ReactiveNodePyTorch:
    def __init__(self, value=None, compute_fn=None, dependencies=None, can_set=False):
        self.compute_fn = compute_fn
        self.dependencies = dependencies or []
        self._value = value
        self.can_set = can_set

    def set_value(self, value):
        if not self.can_set:
            raise ValueError("Cannot set value on immutable node")
        self._value = value
        # In a simpler PyTorch style, we might not explicitly track dependents for manual recomputation

    def compute(self):
        if self.compute_fn:
            args = [dep.get_value() for dep in self.dependencies]
            if all(arg is not None for arg in args):
                self._value = self.compute_fn(*args)
        return self._value

    def get_value(self):
        if self.compute_fn:
            return self.compute()
        return self._value

    def override(self, temp_value):
        # Simplified override for demonstration
        class OverrideContext:
            def __init__(self_inner, node):
                self_inner.node = node
                self_inner.original_value = None

            def __enter__(self_inner):
                self_inner.original_value = self_inner.node._value
                self_inner.node._value = temp_value
                return self_inner.node

            def __exit__(self_inner, exc_type, exc_val, exc_tb):
                self_inner.node._value = self_inner.original_value

        return OverrideContext(self)

    def __repr__(self):
        return f"ReactiveNodePyTorch(value={self._value}, can_set={self.can_set})"

class WubGraphPyTorch:
    def __init__(self):
        self.inputs = {}

    def input(self, name, value=None):
        node = ReactiveNodePyTorch(value=value, can_set=True)
        self.inputs[name] = node
        return node

    def fn(self, func):
        def wrapper(*args):
            dep_nodes = [arg for arg in args if isinstance(arg, ReactiveNodePyTorch)]
            return ReactiveNodePyTorch(compute_fn=lambda *a: func(*a), dependencies=dep_nodes)
        return wrapper

# --- PyTorch-style Example (Adjusted) ---

wub_pt = WubGraphPyTorch()

spot_pt = wub_pt.input("spot", value=100.0)
fx_pt = wub_pt.input("fx", value=0.95)

@wub_pt.fn
def usd_price_pt(spot, fx):
    print(f"PyTorch-style USD price: {spot} * {fx} = {spot * fx}")
    return spot * fx

usd_pt = usd_price_pt(spot_pt, fx_pt)

print(f"USD price: {usd_pt.get_value()}")

with fx_pt.override(1.0) as overridden_fx:
    print(f"USD price: {usd_pt.get_value()}")

print(f"USD price: {usd_pt.get_value()}") # Original value restored