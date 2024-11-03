import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score
import jax
import jax.numpy as jnp
from flax import linen as nn
from flax.training import train_state
import optax

# Simulate some stock data
np.random.seed(0)
data_length = 10000
data = np.random.randn(data_length)

def create_dataset(data, n_steps_in, n_steps_out):
    X, y = [], []
    for i in range(len(data) - n_steps_in - n_steps_out + 1):
        X.append(data[i:(i + n_steps_in)])
        y.append(data[(i + n_steps_in):(i + n_steps_in + n_steps_out)])
    return np.array(X), np.array(y)

# Define the input and output sequence length
n_steps_in, n_steps_out = 60, 5
X, y = create_dataset(data, n_steps_in, n_steps_out)
y = y.mean(axis=1)  # Predicting the average return of the next 5 minutes

# Train-test split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

class LSTMModel(nn.Module):
    hidden_dim: int  # Number of LSTM units

    @nn.compact
    def __call__(self, x):
        x = x[:, :, None]  # Add feature dimension
        lstm_cell = nn.LSTMCell(features=self.hidden_dim)
        
        batch_size = x.shape[0]
        lstm_state = lstm_cell.initialize_carry(jax.random.PRNGKey(0), (batch_size,))
        
        for i in range(x.shape[1]):
            lstm_state, y = lstm_cell(lstm_state, x[:, i, :])
        
        return nn.Dense(features=1, dtype=jnp.float32)(y)

def create_train_state(rng, learning_rate, hidden_dim):
    model = LSTMModel(hidden_dim=hidden_dim)
    params = model.init(rng, jnp.ones((1, n_steps_in)))['params']
    tx = optax.adam(learning_rate)
    return train_state.TrainState.create(apply_fn=model.apply, params=params, tx=tx)

# Initialize the model
hidden_dim = 64
learning_rate = 0.001
rng = jax.random.PRNGKey(0)
state = create_train_state(rng, learning_rate, hidden_dim)

@jax.jit
def train_step(state, batch):
    def loss_fn(params):
        predictions = state.apply_fn({'params': params}, batch[0])
        return jnp.mean((predictions.squeeze() - batch[1]) ** 2)
    
    loss, grads = jax.value_and_grad(loss_fn)(state.params)
    state = state.apply_gradients(grads=grads)
    return state, loss

# Training loop
num_epochs = 50
batch_size = 32

for epoch in range(num_epochs):
    # Create a new random permutation of the training data
    perm = jax.random.permutation(jax.random.PRNGKey(epoch), len(X_train))
    # Use this permutation to shuffle both X_train and y_train
    X_train_shuffled = X_train[perm]
    y_train_shuffled = y_train[perm]
    
    for i in range(0, len(X_train), batch_size):
        batch = (X_train_shuffled[i:i+batch_size], y_train_shuffled[i:i+batch_size])
        state, loss = train_step(state, batch)
    
    print(f"Epoch {epoch}, Loss: {loss}")

def predict(params, X):
    return LSTMModel(hidden_dim=hidden_dim).apply({'params': params}, X).squeeze()

y_pred = predict(state.params, X_test)
r2 = r2_score(y_test, y_pred)
print(f"R-squared: {r2}")