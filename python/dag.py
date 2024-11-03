import networkx as nx
from concurrent.futures import ThreadPoolExecutor, as_completed
from collections import defaultdict
import time

def execute_task(task):
    """Simulate task execution."""
    print(f"Starting task: {task}")
    time.sleep(1)  # Simulate task duration
    print(f"Completed task: {task}")
    return task

def run_tasks_with_dependencies(task_dependencies):
    """
    Execute tasks in topologically sorted order using a worker pool, completing tasks as they finish.

    Parameters:
    - task_dependencies (dict): A dictionary where keys are task names and values are lists of task dependencies.
    """
    # Create a directed graph for tasks and dependencies
    dag = nx.DiGraph()
    for task, dependencies in task_dependencies.items():
        for dep in dependencies:
            dag.add_edge(dep, task)
    
    # Ensure the graph is a DAG
    if not nx.is_directed_acyclic_graph(dag):
        raise ValueError("The input graph is not a DAG; topological sorting is not possible.")
    
    # Determine initial tasks (those with no dependencies)
    in_degree = {task: len(list(dag.predecessors(task))) for task in dag.nodes}
    ready_tasks = [task for task, degree in in_degree.items() if degree == 0]

    # Dictionary to hold tasks waiting on other tasks
    waiting_tasks = defaultdict(list)
    for task in dag.nodes:
        for successor in dag.successors(task):
            waiting_tasks[task].append(successor)
    
    # Use ThreadPoolExecutor to execute tasks concurrently
    with ThreadPoolExecutor() as executor:
        futures = {executor.submit(execute_task, task): task for task in ready_tasks}
        print(f"Initial tasks submitted: {ready_tasks}")
        
        while futures:
            # Process tasks as they complete
            for future in as_completed(futures):
                completed_task = futures.pop(future)
                print(f"Task {completed_task} finished.")
                
                # Check if any dependent tasks can now start
                for dependent in waiting_tasks[completed_task]:
                    in_degree[dependent] -= 1
                    if in_degree[dependent] == 0:
                        print(f"Submitting dependent task: {dependent}")
                        futures[executor.submit(execute_task, dependent)] = dependent


if __name__ == "__main__":
    import time 
    s = time.monotonic()
    task_dependencies = {
        'task1': [],
        'task2': ['task1'],
        'task3': ['task1'],
        'task4': ['task2', 'task3'],
        'task5': ['task4']
    }

    run_tasks_with_dependencies(task_dependencies)
    print(f"finished in {time.monotonic() - s} seconds")