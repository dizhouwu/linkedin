from collections import defaultdict
from datetime import date
from enum import Enum

class Mode(Enum):
    CACHED = "cached"
    COMPUTED = "computed"

class ReactiveNode:
    def __init__(self, value=None, compute_fn=None, dependencies=None, can_set=False):
        self.compute_fn = compute_fn
        self.dependencies = dependencies or []
        self.dependents = []
        self._value = value
        self._is_dirty = True
        self._override_stack = []
        self.can_set = can_set

        for dep in self.dependencies:
            dep.dependents.append(self)

    def set_value(self, value):
        if not self.can_set:
            raise ValueError("Cannot set value on immutable node")
        self._value = value
        self.mark_dependents_dirty()

    def get_value(self):
        if self._override_stack:
            return self._override_stack[-1]
        if self.compute_fn and self._is_dirty:
            self.recompute()
        return self._value

    def recompute(self):
        args = [dep.get_value() for dep in self.dependencies]
        if any(arg is None for arg in args):
            return
        self._value = self.compute_fn(*args)
        self._is_dirty = False

    def mark_dirty(self):
        if not self._is_dirty:
            self._is_dirty = True
        for dep in self.dependents:
            dep.mark_dirty()

    def mark_dependents_dirty(self):
        for dep in self.dependents:
            dep.mark_dirty()

    def override(self, temp_value):
        node = self

        class OverrideContext:
            def __enter__(self_inner):
                node._override_stack.append(temp_value)
                node.mark_dependents_dirty()
                return node

            def __exit__(self_inner, exc_type, exc_val, exc_tb):
                node._override_stack.pop()
                node.mark_dependents_dirty()

        return OverrideContext()

    def __repr__(self):
        return f"ReactiveNode(value={self.get_value()}, dirty={self._is_dirty}, can_set={self.can_set})"


class WubGraph:
    def __init__(self):
        self.inputs = defaultdict(dict)

    def input(self, name, dt, value=None):
        node = ReactiveNode(value=value, can_set=True)
        self.inputs[name][dt] = node
        return node

    def get_input(self, name, dt):
        return self.inputs[name].get(dt)

    def fn(self, mode, can_set=False):
        def decorator(func):
            def wrapper(*args, **kwargs):
                if mode == Mode.CACHED:
                    # Assume dt is last arg or kwarg
                    dt = kwargs.get("dt", args[-1] if args else None)
                    name = func.__name__
                    def load():
                        return self.load_from_disk(name, dt)
                    return ReactiveNode(compute_fn=load, can_set=can_set)
                elif mode == Mode.COMPUTED:
                    dep_nodes = [arg for arg in args if isinstance(arg, ReactiveNode)]
                    return ReactiveNode(compute_fn=lambda *a: func(*a), dependencies=dep_nodes, can_set=can_set)
                else:
                    raise ValueError(f"Unknown mode: {mode}")
            return wrapper
        return decorator

    def load_from_disk(self, name, dt):
        print(f"Loading {name}@{dt} from disk...")
        if name == "spot_price":
            return 100.0
        elif name == "fx_rate":
            return 0.95
        return 42.0


# --- Example usage ---

wub = WubGraph()
dt = date(2025, 5, 10)

@wub.fn(Mode.CACHED)
def spot_price(dt):
    pass

@wub.fn(Mode.CACHED,can_set=True)
def fx_rate(dt):
    pass

@wub.fn(Mode.COMPUTED)
def usd_price(spot, fx):
    print(f"USD price: {spot} * {fx} = {spot * fx}")
    return spot * fx

spot = spot_price(dt)
fx = fx_rate(dt)
usd = usd_price(spot, fx)

print(usd.get_value())

with fx.override(1.0):
    print(usd.get_value())