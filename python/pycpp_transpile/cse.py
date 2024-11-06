import ast
import astor
from collections import defaultdict, deque

# Original code to be optimized
source_code = """
x = 4
y = 5
z = x * y
q = x * y
w = x
u = w * y
"""

# Step 1: Parse the source code into an AST
tree = ast.parse(source_code)

# Step 2: Perform CSE on the AST
class CSETransformer(ast.NodeTransformer):
    def __init__(self):
        self.subexprs = {}
        self.new_assignments = []

    def visit_BinOp(self, node):
        # Visit the left and right to get the optimized nodes
        self.generic_visit(node)

        # Create a unique key for the subexpression
        key = (type(node.op), ast.dump(node.left), ast.dump(node.right))

        if key in self.subexprs:
            # If the subexpression has already been seen, replace it with the variable
            return ast.Name(id=self.subexprs[key], ctx=ast.Load())
        else:
            # If it’s a new subexpression, create a new variable name
            var_name = f"subexpr_{len(self.subexprs)}"
            self.subexprs[key] = var_name

            # Create a new assignment for the subexpression
            new_assign = ast.Assign(targets=[ast.Name(id=var_name, ctx=ast.Store())],
                                    value=node)
            self.new_assignments.append(new_assign)
            return ast.Name(id=var_name, ctx=ast.Load())

    def visit_Assign(self, node):
        # Transform the right-hand side of the assignment
        self.generic_visit(node)
        return node

# Create an instance of the transformer and optimize the AST
transformer = CSETransformer()
optimized_tree = transformer.visit(tree)

# Step 3: Collect dependencies
dependencies = defaultdict(set)
assignment_map = {}

# Track dependencies for each assignment
for node in optimized_tree.body + transformer.new_assignments:
    if isinstance(node, ast.Assign):
        target = node.targets[0].id
        # Extract variables on the RHS
        used_vars = {n.id for n in ast.walk(node.value) if isinstance(n, ast.Name)}
        dependencies[target].update(used_vars)
        assignment_map[target] = node

# Step 4: Build the dependency graph and perform a topological sort
# Calculate in-degrees and construct graph
in_degree = defaultdict(int)
graph = defaultdict(list)

for var, deps in dependencies.items():
    for dep in deps:
        graph[dep].append(var)
        in_degree[var] += 1

# Topological sort using Kahn's algorithm
sorted_assignments = []
queue = deque([node for node in assignment_map if in_degree[node] == 0])

while queue:
    var = queue.popleft()
    sorted_assignments.append(assignment_map[var])
    for neighbor in graph[var]:
        in_degree[neighbor] -= 1
        if in_degree[neighbor] == 0:
            queue.append(neighbor)

# Step 5: Compile the optimized AST back to code in sorted order
optimized_tree.body = sorted_assignments

# Step 6: Generate and print the optimized code
optimized_code = astor.to_source(optimized_tree)
print("Optimized Code:\n", optimized_code)

# Step 7: Execute the optimized code
exec_locals = {}
exec(optimized_code, {}, exec_locals)

# Print results of the executed code
for key, value in exec_locals.items():
    print(f"{key} = {value}")
