import time
from typing import Dict, List, Callable, Any, Optional, Tuple, Set, Union
import logging
from enum import Enum
from collections import defaultdict
import functools

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)

class ExecutionMode(Enum):
    """Execution strategies for task nodes"""
    EAGER = "eager"     # Compute as soon as dependencies are satisfied
    LAZY = "lazy"       # Compute only when result is requested
    CACHED = "cached"   # Load from cache/disk first if available

class TaskNode:
    """
    Represents a reactive computation node in a directed acyclic graph.
    
    Each node can be eager (computed during traversal) or lazy (computed on demand),
    and tracks its dependencies and dependents to maintain the reactive graph structure.
    """
    def __init__(
        self, 
        name: str, 
        func: Callable, 
        dependencies: Optional[List['TaskNode']] = None, 
        mode: ExecutionMode = ExecutionMode.EAGER,
        can_set: bool = False
    ):
        """
        Initialize a task node with a name, function, dependencies, and execution mode.
        
        Args:
            name: Unique identifier for the task
            func: Function to execute when the task runs
            dependencies: List of task nodes this task depends on
            mode: Execution strategy (EAGER, LAZY, or CACHED)
            can_set: Whether this node's value can be manually set
        """
        self.name = name
        self.func = func
        self.dependencies = dependencies or []
        self.dependents = []
        self.mode = mode
        self.can_set = can_set
        
        # Node state
        self.result = None
        self.is_computed = False
        self.is_dirty = True
        self._override_stack = []
        
        # Register this node as a dependent of each dependency
        for dep in self.dependencies:
            dep.dependents.append(self)

    def compute(self, force: bool = False) -> Any:
        """
        Execute the node's function with resolved dependency values.
        
        Args:
            force: If True, recompute even if node is not dirty
            
        Returns:
            The result of executing the node function
        """
        # Skip if clean and not forced
        if not force and not self.is_dirty and self.is_computed:
            return self.result
            
        # Get dependency values
        dependency_values = [dep.get_value() for dep in self.dependencies]
        
        # Check if any dependencies returned None
        if any(val is None for val in dependency_values):
            logger.debug(f"Node '{self.name}': dependency returned None, skipping computation.")
            return None
            
        # Log what's happening
        logger.info(f"Computing: Node '{self.name}' ({self.mode.value}) with inputs: {dependency_values}")
        
        # Small delay to make execution order clearer in logs
        time.sleep(0.05)
        
        # Compute the result
        self.result = self.func(*dependency_values)
        self.is_computed = True
        self.is_dirty = False
        
        logger.info(f"Completed: Node '{self.name}' → {self.result}")
        return self.result

    def get_value(self) -> Any:
        """
        Get the current value of this node, computing it if necessary.
        
        Returns:
            The computed result of the node
        """
        # Check for override values first
        if self._override_stack:
            return self._override_stack[-1]
            
        # For lazy and cached nodes, compute only when needed and dirty
        if self.is_dirty and (self.mode == ExecutionMode.LAZY or self.mode == ExecutionMode.CACHED):
            self.compute()
            
        return self.result

    def set_value(self, value: Any) -> None:
        """
        Manually set the value of this node.
        
        Args:
            value: The value to set
        
        Raises:
            ValueError: If this node doesn't allow setting values
        """
        if not self.can_set:
            raise ValueError(f"Cannot set value on immutable node '{self.name}'")
            
        logger.info(f"Setting value on node '{self.name}': {value}")
        self.result = value
        self.is_computed = True
        self.is_dirty = False
        self.mark_dependents_dirty()

    def override(self, temp_value: Any):
        """
        Temporarily override the value of this node.
        
        Args:
            temp_value: The temporary value to use
            
        Returns:
            A context manager that restores the original state when exiting
        """
        node = self

        class OverrideContext:
            def __enter__(self):
                node._override_stack.append(temp_value)
                node.mark_dependents_dirty()
                return node

            def __exit__(self, exc_type, exc_val, exc_tb):
                node._override_stack.pop()
                node.mark_dependents_dirty()

        return OverrideContext()

    def mark_dirty(self) -> None:
        """Mark this node and all its dependents as dirty."""
        if not self.is_dirty:
            self.is_dirty = True
            
        for dep in self.dependents:
            dep.mark_dirty()

    def mark_dependents_dirty(self) -> None:
        """Mark all dependents of this node as dirty."""
        for dep in self.dependents:
            dep.mark_dirty()

    def reset(self) -> None:
        """Reset the node's computed state and result."""
        if self.is_computed:
            logger.debug(f"Resetting node '{self.name}'")
            self.is_computed = False
            self.is_dirty = True
            self._override_stack = []

    def __repr__(self) -> str:
        """String representation of this node."""
        value = self.get_value() if self.is_computed else "Not computed"
        return f"TaskNode(name='{self.name}', value={value}, mode={self.mode.value}, dirty={self.is_dirty})"


class DAGRegistry:
    """
    Registry and factory for creating and managing task nodes in a DAG.
    """
    def __init__(self):
        """Initialize a new DAG registry."""
        self.nodes = {}
        self.input_nodes = defaultdict(dict)
        self.cached_values = {}

    def node(self, name: Optional[str] = None, dependencies: Optional[List[Union[str, TaskNode]]] = None, 
             mode: ExecutionMode = ExecutionMode.EAGER, can_set: bool = False):
        """
        Decorator for registering functions as task nodes in the DAG.
        
        Args:
            name: Optional name for the node (defaults to function name)
            dependencies: List of nodes or node names this node depends on
            mode: Execution mode (EAGER, LAZY, or CACHED)
            can_set: Whether this node's value can be manually set
            
        Returns:
            Decorated function
        """
        def decorator(func):
            # Use function name if no name provided
            node_name = name or func.__name__
            
            # Resolve dependencies
            dep_nodes = []
            if dependencies:
                for dep in dependencies:
                    # Convert string names to actual nodes
                    if isinstance(dep, str) and dep in self.nodes:
                        dep_nodes.append(self.nodes[dep])
                    elif isinstance(dep, TaskNode):
                        dep_nodes.append(dep)
                    else:
                        raise ValueError(f"Invalid dependency: {dep}. Must be a task node or registered name.")
            
            # Create the task node
            @functools.wraps(func)
            def wrapper(*args, **kwargs):
                # For cached nodes, handle date parameter
                if mode == ExecutionMode.CACHED:
                    # Extract date from args or kwargs if available
                    dt = kwargs.get("dt", args[0] if args else None)
                    
                    # Create a unique node name with the date
                    unique_name = f"{node_name}_{dt}" if dt else node_name
                    
                    # Return existing node if already created
                    if unique_name in self.nodes:
                        return self.nodes[unique_name]
                    
                    # Create a load function for cached values
                    def load_func():
                        cache_key = f"{node_name}_{dt}"
                        if cache_key in self.cached_values:
                            logger.info(f"Loading cached value for '{node_name}' at {dt}")
                            return self.cached_values[cache_key]
                        logger.info(f"No cached value for '{node_name}' at {dt}, computing...")
                        result = func(*args, **kwargs)
                        self.cached_values[cache_key] = result
                        return result
                        
                    # Create and register the node
                    node = TaskNode(
                        name=unique_name,
                        func=load_func,
                        dependencies=[],  # Cached nodes don't have direct dependencies
                        mode=mode,
                        can_set=can_set
                    )
                    
                    # Register the node
                    self.nodes[unique_name] = node
                    return node
                
                # For regular nodes, handle normal call case
                if node_name in self.nodes:
                    return self.nodes[node_name]
                
                # Handle extra dependencies from args
                actual_deps = list(dep_nodes)  # Copy the original dependencies
                for arg in args:
                    if isinstance(arg, TaskNode) and arg not in actual_deps:
                        actual_deps.append(arg)
                
                # Create compute function
                def compute_func(*dep_values):
                    return func(*dep_values)
                
                # Create and register node
                node = TaskNode(
                    name=node_name,
                    func=compute_func,
                    dependencies=actual_deps,
                    mode=mode,
                    can_set=can_set
                )
                
                self.nodes[node_name] = node
                return node
                
            # Add metadata for easier inspection
            wrapper.node_name = node_name
            wrapper.mode = mode
            wrapper.is_dag_node = True
            
            return wrapper
            
        return decorator

    def input(self, name: str, dt, value: Any = None):
        """
        Create an input node that can be set externally.
        
        Args:
            name: Name of the input
            dt: Date or time reference
            value: Initial value (optional)
            
        Returns:
            TaskNode for this input
        """
        # Create a node name that includes the date
        node_name = f"{name}_{dt}"
        
        # Create a node that just returns its value
        node = TaskNode(
            name=node_name,
            func=lambda: None,  # Function is irrelevant for input nodes
            dependencies=[],
            mode=ExecutionMode.EAGER,
            can_set=True
        )
        
        # Set initial value if provided
        if value is not None:
            node.set_value(value)
            
        # Store in inputs dict
        self.input_nodes[name][dt] = node
        self.nodes[node_name] = node
        
        return node

    def get_input(self, name: str, dt):
        """
        Retrieve an input node by name and date.
        
        Args:
            name: Name of the input
            dt: Date or time reference
            
        Returns:
            The input node if found, None otherwise
        """
        return self.input_nodes[name].get(dt)

    def fn(self, mode: ExecutionMode = ExecutionMode.EAGER, can_set: bool = False):
        """
        Legacy decorator alias to maintain compatibility with original code.
        
        Args:
            mode: Execution mode (EAGER, LAZY, or CACHED)
            can_set: Whether this node's value can be manually set
            
        Returns:
            Node decorator
        """
        return self.node(mode=mode, can_set=can_set)

    def depends_on(self, *deps, mode: ExecutionMode = ExecutionMode.EAGER, can_set: bool = False):
        """
        Decorator for specifying node dependencies more cleanly.
        
        Args:
            *deps: Variable number of node dependencies (nodes or names)
            mode: Execution mode (EAGER, LAZY, or CACHED)
            can_set: Whether this node's value can be manually set
            
        Returns:
            Node decorator
        """
        # Make sure all dependency nodes are properly registered
        dep_nodes = []
        for dep in deps:
            if isinstance(dep, TaskNode):
                # Check that the TaskNode is actually in our registry
                if dep.name not in self.nodes:
                    self.nodes[dep.name] = dep
                dep_nodes.append(dep)
            elif isinstance(dep, str) and dep in self.nodes:
                dep_nodes.append(self.nodes[dep])
            else:
                raise ValueError(f"Invalid dependency: {dep}. Must be a task node or registered name.")
                
        def decorator(func):
            node_name = func.__name__
            
            @functools.wraps(func)
            def compute_func(*dep_values):
                return func(*dep_values)
                
            # Create and register the node right away
            node = TaskNode(
                name=node_name,
                func=compute_func,
                dependencies=dep_nodes,
                mode=mode,
                can_set=can_set
            )
            
            self.nodes[node_name] = node
            
            # Return a wrapper function that always returns the node
            @functools.wraps(func)
            def wrapper(*args, **kwargs):
                return node
                
            return wrapper
            
        return decorator

    def reset(self):
        """Reset all nodes in the registry."""
        for node in self.nodes.values():
            node.reset()
        self.cached_values = {}


class DAG:
    """
    Manages and executes a directed acyclic graph (DAG) of tasks.
    
    Supports mixed eager and lazy evaluation strategies using the TaskNode objects.
    """
    def __init__(self, registry: DAGRegistry):
        """
        Initialize the DAG with a registry of nodes.
        
        Args:
            registry: The registry containing task nodes
        """
        self.registry = registry
        self.eager_executed = False

    def _build_graph(self) -> Tuple[Dict[str, List[str]], Dict[str, int]]:
        """
        Build adjacency list and in-degree map for topological sorting.
        
        Returns:
            Tuple of (adjacency_list, in_degree_map)
        """
        nodes = self.registry.nodes
        
        # adjacency_list[node] = list of nodes that depend on it
        adjacency_list = {name: [] for name in nodes}
        
        # in_degree[node] = number of nodes it depends on
        in_degree = {name: 0 for name in nodes}
        
        for node_name, node in nodes.items():
            in_degree[node_name] = len(node.dependencies)
            for dep in node.dependencies:
                adjacency_list[dep.name].append(node_name)
                
        return adjacency_list, in_degree

    def _topological_sort(self) -> List[str]:
        """
        Perform topological sort of the node graph using Kahn's algorithm.
        
        Returns:
            List of node names in topological order
            
        Raises:
            ValueError: If a cycle is detected in the graph
        """
        adjacency_list, in_degree = self._build_graph()
        
        # Start with nodes that have no dependencies
        queue = [name for name, degree in in_degree.items() if degree == 0]
        sorted_nodes = []
        
        while queue:
            current_node = queue.pop(0)
            sorted_nodes.append(current_node)
            
            # Update in-degrees of dependent nodes
            for dependent_node in adjacency_list[current_node]:
                in_degree[dependent_node] -= 1
                if in_degree[dependent_node] == 0:
                    queue.append(dependent_node)
                    
        # Check for cycles
        if len(sorted_nodes) != len(self.registry.nodes):
            missing_nodes = set(self.registry.nodes.keys()) - set(sorted_nodes)
            raise ValueError(f"Cycle detected in graph. Nodes in cycle: {missing_nodes}")
            
        return sorted_nodes

    def run_eager_nodes(self) -> None:
        """
        Execute all eager nodes in topological order.
        
        Tasks marked as LAZY will only compute when their value is requested.
        Tasks marked as EAGER will compute during this phase.
        """
        if self.eager_executed:
            logger.debug("Eager nodes already executed, skipping.")
            return
            
        logger.info("Starting eager execution phase")
        
        try:
            topo_order = self._topological_sort()
            logger.info(f"Topological order: {topo_order}")
            
            # Process eager nodes in topological order
            for node_name in topo_order:
                node = self.registry.nodes[node_name]
                if node.mode == ExecutionMode.EAGER and not node.is_computed:
                    logger.info(f"Processing eager node '{node_name}'")
                    node.compute()
                    
        except ValueError as e:
            logger.error(f"Error during eager execution: {e}")
            return
            
        logger.info("Completed eager execution phase")
        self.eager_executed = True

    def get_node_value(self, node_name: str) -> Any:
        """
        Get the value of a node, computing it if necessary.
        
        Args:
            node_name: Name of the node to get value for
            
        Returns:
            The computed value of the node
        """
        if node_name not in self.registry.nodes:
            raise ValueError(f"Node '{node_name}' not found in DAG")
            
        return self.registry.nodes[node_name].get_value()

    def execute(self, target_node: str) -> Any:
        """
        Execute the DAG and return the result of the target node.
        
        Args:
            target_node: Name of the node whose result should be returned
            
        Returns:
            The result of the target node
            
        Raises:
            ValueError: If target_node doesn't exist or the graph has cycles
        """
        if target_node not in self.registry.nodes:
            raise ValueError(f"Target node '{target_node}' not found in DAG")
            
        logger.info(f"Executing DAG with target: {target_node}")
        
        # First run eager nodes
        self.run_eager_nodes()
        
        # Then get the value of the target node (will compute lazy dependencies as needed)
        logger.info(f"Computing target node: {target_node}")
        result = self.get_node_value(target_node)
        
        self._print_state()
        
        return result
        
    def reset(self) -> None:
        """Reset the state of all nodes."""
        self.registry.reset()
        self.eager_executed = False
            
    def _print_state(self) -> None:
        """Print the current state of all nodes."""
        logger.info("Current state of all nodes:")
        for name, node in sorted(self.registry.nodes.items()):
            status = "Computed" if node.is_computed else "Not Computed"
            value = node.get_value() if node.is_computed else "N/A"
            mode = node.mode.value
            logger.info(f"Node: {name:<20} ({mode:<5}), Status: {status:<12}, Value: {value}")


# Create global registry and DAG instances
dag = DAGRegistry()
graph = DAG(dag)

# ------------------------------------------------------
# Example usage with the hybrid eager/lazy computation model
# ------------------------------------------------------

if __name__ == "__main__":
    import datetime

    # Reset for clean demo
    dag.reset()
    graph.reset()

    # Example 1: Basic eager/lazy computation
    print("\n=== Example 1: Basic Eager/Lazy Computation ===")

    @dag.node(mode=ExecutionMode.EAGER)
    def e1():
        print("  Computing e1 (eager source node)")
        return 10

    @dag.node(mode=ExecutionMode.LAZY)
    def l1():
        print("  Computing l1 (lazy source node)")
        return 100

    @dag.depends_on(e1(), mode=ExecutionMode.EAGER)
    def e2(e1_val):
        print(f"  Computing e2 from e1={e1_val}")
        return e1_val + 1

    @dag.depends_on(l1(), mode=ExecutionMode.LAZY)
    def l2(l1_val):
        print(f"  Computing l2 from l1={l1_val}")
        return l1_val + 10

    @dag.depends_on(l1(), mode=ExecutionMode.EAGER)
    def e3(l1_val):
        print(f"  Computing e3 from l1={l1_val}")
        return l1_val * 2

    @dag.depends_on(e1(), mode=ExecutionMode.LAZY)
    def l3(e1_val):
        print(f"  Computing l3 from e1={e1_val}")
        return e1_val * 3

    @dag.depends_on(e2(), e3(), mode=ExecutionMode.EAGER)
    def e4(e2_val, e3_val):
        print(f"  Computing e4 from e2={e2_val}, e3={e3_val}")
        return e2_val + e3_val

    # Get references to the nodes first
    l2_node = l2()
    l3_node = l3()
    e4_node = e4()
    
    # Debug print to verify nodes exist
    print(f"  Node references: l2={l2_node.name}, l3={l3_node.name}, e4={e4_node.name}")
    
    # Create l4 node with explicit dependencies
    @dag.node(name="l4", dependencies=[l2_node, l3_node, e4_node], mode=ExecutionMode.LAZY)
    def l4(l2_val, l3_val, e4_val):
        print(f"  Computing l4 from l2={l2_val}, l3={l3_val}, e4={e4_val}")
        return l2_val + l3_val + e4_val
    
    # Directly invoke l4 to ensure node is created and registered
    l4_node = l4()

    # Debug what nodes are in the registry
    print("\nNodes in registry before execution:")
    for node_name in dag.nodes:
        print(f"  - {node_name}")
        
    # Just to be double sure, create a reference to l4 node
    l4_ref = l4()
    print(f"l4 node created with name: {l4_ref.name}")

    # Execute graph with target l4
    result = graph.execute("l4")
    print(f"\nFinal result for l4: {result}")

    # Expected:
    # e1 = 10 (eager)
    # l1 = 100 (lazy, forced by e3)
    # e2 = e1 + 1 = 11 (eager)
    # e3 = l1 * 2 = 200 (eager, forces l1)
    # e4 = e2 + e3 = 11 + 200 = 211 (eager)
    # When l4 is requested:
    # l2 = l1 + 10 = 110 (lazy)
    # l3 = e1 * 3 = 30 (lazy)
    # l4 = l2 + l3 + e4 = 110 + 30 + 211 = 351 (lazy)

    # Example 2: Using inputs and reactive updates
    print("\n\n=== Example 2: Reactive Updates with Inputs ===")

    # Reset for a new example
    dag.reset()
    graph.reset()

    # Create some input nodes
    dt = datetime.date(2025, 5, 10)
    spot_price = dag.input("spot_price", dt, 100.0)
    fx_rate = dag.input("fx_rate", dt, 0.95)

    # Create computed nodes
    @dag.depends_on(spot_price, fx_rate, mode=ExecutionMode.EAGER)
    def usd_price(spot, fx):
        print(f"  Computing USD price: {spot} * {fx} = {spot * fx}")
        return spot * fx

    # First computation
    print("\nInitial computation:")
    result = graph.execute("usd_price")
    print(f"USD Price: {result}")

    # Update input and see reactive update
    print("\nUpdating spot price to 110.0:")
    spot_price.set_value(110.0)
    result = graph.execute("usd_price")
    print(f"New USD Price: {result}")

    # Use override for temporary changes
    print("\nTemporarily overriding FX rate to 1.0:")
    with fx_rate.override(1.0):
        result = graph.execute("usd_price")
        print(f"USD Price with overridden FX: {result}")

    # Check that value returns after override
    print("\nAfter override ends:")
    result = graph.execute("usd_price")
    print(f"USD Price back to normal: {result}")

    # Example 3: Using cached nodes
    print("\n\n=== Example 3: Using Cached and Computed Nodes ===")

    # Reset for a new example
    dag.reset()
    graph.reset()

    # Create cached nodes
    @dag.node(mode=ExecutionMode.CACHED)
    def market_data(dt):
        print(f"  Loading market data for {dt}")
        return {"price": 100.0, "volume": 1000} 

    @dag.node(mode=ExecutionMode.CACHED)
    def exchange_rate(dt):
        print(f"  Loading exchange rate for {dt}")
        return 0.9

    # Create computed nodes that use cached data
    @dag.depends_on(market_data(dt), exchange_rate(dt))
    def market_value(data, rate):
        print(f"  Computing market value: {data['price']} * {data['volume']} * {rate}")
        return data["price"] * data["volume"] * rate

    # Execute
    result = graph.execute("market_value")
    print(f"Market value: {result}")

    # Rerun - should use cached values
    print("\nRerunning (should use cached values):")
    result = graph.execute("market_value")
    print(f"Market value (recomputed): {result}")