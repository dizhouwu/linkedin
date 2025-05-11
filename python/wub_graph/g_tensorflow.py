from collections import defaultdict
from datetime import date
from enum import Enum
from collections import deque

class Mode(Enum):
    CACHED = "cached"
    COMPUTED = "computed"

class SymbolicNodeRaw:
    def __init__(self, name, operation=None, inputs=None):
        self.name = name
        self.operation = operation
        self.inputs = inputs or []
        self.output = None  # Will hold the result during execution

    def __repr__(self):
        return f"SymbolicNodeRaw(name='{self.name}', op={self.operation.__name__ if self.operation else 'Input'})"

class WubGraphRawTFStyle:
    def __init__(self):
        self.symbolic_inputs = {}
        self.symbolic_nodes = {}

    def input(self, name):
        symbolic_input = SymbolicNodeRaw(name=name)
        self.symbolic_inputs[name] = symbolic_input
        self.symbolic_nodes[name] = symbolic_input
        return symbolic_input

    def fn(self, func):
        def wrapper(*arg_names):
            input_nodes = [self.symbolic_nodes[name] for name in arg_names]
            symbolic_output = SymbolicNodeRaw(name=func.__name__, operation=func, inputs=input_nodes)
            self.symbolic_nodes[func.__name__] = symbolic_output
            return symbolic_output
        return wrapper

    def _build_dependency_graph(self):
        dependency_graph = defaultdict(list)
        in_degree = defaultdict(int)
        for node in self.symbolic_nodes.values():
            for input_node in node.inputs:
                dependency_graph[input_node].append(node)
                in_degree[node] += 1
        return dependency_graph, in_degree

    def topological_sort(self):
        dependency_graph, in_degree = self._build_dependency_graph()
        dq = deque([node for node in self.symbolic_inputs.values() if in_degree[node] == 0])
        sorted_list = []

        while dq:
            node = dq.popleft()
            sorted_list.append(node)
            for neighbor in dependency_graph[node]:
                in_degree[neighbor] -= 1
                if in_degree[neighbor] == 0:
                    dq.append(neighbor)

        if len(sorted_list) != len(self.symbolic_nodes):
            raise ValueError("Cycle detected in the computation graph")

        return sorted_list

    def execute(self, feed_dict):
        node_outputs = {}
        sorted_nodes = self.topological_sort()

        # Initialize outputs of input nodes
        for name, value in feed_dict.items():
            if name in self.symbolic_inputs:
                node_outputs[self.symbolic_inputs[name]] = value
            else:
                raise ValueError(f"Missing input value for '{name}'")

        # Execute operations in topological order
        for node in sorted_nodes:
            if node.operation:
                input_values = [node_outputs[input_node] for input_node in node.inputs]
                result = node.operation(*input_values)
                node_outputs[node] = result

        # Collect results by node name
        results = {node.name: node_outputs.get(node) for node in self.symbolic_nodes.values()}
        return results

# --- Raw Python TensorFlow-style Example (Using Topological Sort) ---

wub_raw_tf = WubGraphRawTFStyle()

spot_sym_raw = wub_raw_tf.input("spot")
fx_sym_raw = wub_raw_tf.input("fx")

@wub_raw_tf.fn
def usd_price_raw(spot, fx):
    print(f"Raw Python TF-style USD price: {spot} * {fx} = {spot * fx}")
    return spot * fx

usd_sym_raw = usd_price_raw("spot", "fx")

# 1. Define the graph (lazily)
print("Defined symbolic graph:")
for name, node in wub_raw_tf.symbolic_nodes.items():
    print(f"- {node}")

# 2. Execute the graph with data
feed_dict_raw = {"spot": 100.0, "fx": 0.95}
results_raw = wub_raw_tf.execute(feed_dict_raw)
print(f"USD price: {results_raw['usd_price_raw']}")

feed_dict_override_raw = {"spot": 100.0, "fx": 1.0}
results_override_raw = wub_raw_tf.execute(feed_dict_override_raw)
print(f"USD price: {results_override_raw['usd_price_raw']}")