# Weights & Biases ML Query Optimizer

This example demonstrates integrating Weights & Biases (W&B) experiment tracking with a C++ application for machine learning-based query optimization in SimpleDB.

## Overview

The ML Query Optimizer uses machine learning techniques to optimize database query execution. It integrates with Weights & Biases to track:

- Training metrics (loss, accuracy, learning rate)
- Query execution times
- Optimization scores
- Model performance

## Prerequisites

```bash
# Install Python and W&B
sudo apt-get install -y python3 python3-pip
pip3 install wandb

# Login to W&B
wandb login YOUR_API_KEY
```

## Building

```bash
mkdir -p build
cd build
cmake ..
make
```

## Running

```bash
# Set W&B API key (if not logged in)
export WANDB_API_KEY="your-api-key"

# Run the optimizer
./ml_query_optimizer
```

## Features

- **Training Mode**: Trains the query optimization model over multiple epochs
- **Testing Mode**: Applies optimization to sample queries
- **W&B Integration**: Automatically logs all metrics to your W&B dashboard
- **Real-time Monitoring**: View training progress in real-time at wandb.ai

## Viewing Results

After running the application, visit:
```
https://wandb.ai/your-username/simpledb-query-optimization
```

You'll see:
- Training loss curves
- Accuracy improvements over epochs
- Query execution time distributions
- Optimization score trends

## Integration with SimpleDB

This example can be integrated with the main SimpleDB engine to provide:
- Intelligent query plan selection
- Automatic index recommendations
- Adaptive query caching
- Performance anomaly detection

## Example Output

```
=== SimpleDB ML Query Optimizer with Weights & Biases ===
W&B Run ID: abc123xyz

--- Training Phase ---
Training query optimization model with 20 epochs...
Epoch 0: loss=1, accuracy=0.5, lr=0.01
Epoch 1: loss=0.5, accuracy=0.75, lr=0.0095
...

--- Testing Phase ---
Query: SELECT * FROM users WHERE age > 25
Optimization score: 0.872

=== Optimization Complete ===
View results at https://wandb.ai/your-username/simpledb-query-optimization
```

## Architecture

```
┌─────────────────┐
│  C++ App        │
│  (Query         │
│   Optimizer)    │
└────────┬────────┘
         │
         │ System calls
         ▼
┌─────────────────┐
│  Python/W&B     │
│  API            │
└────────┬────────┘
         │
         │ HTTPS
         ▼
┌─────────────────┐
│  W&B Cloud      │
│  Dashboard      │
└─────────────────┘
```

## Performance Considerations

- The W&B integration uses system calls to Python, which adds minimal overhead
- For production use, consider batching metrics to reduce API calls
- GPU acceleration is supported when available
- Metrics are logged asynchronously to minimize impact on query execution

## Advanced Usage

### Custom Metrics

```cpp
// Add custom metrics in ml_query_optimizer.cpp
logger.log("cache_hit_rate", 0.85, epoch);
logger.log("memory_usage_mb", 512.0, epoch);
```

### Hyperparameter Tuning

Use W&B Sweeps for automatic hyperparameter optimization:

```yaml
# sweep.yaml
program: ml_query_optimizer
method: bayes
metric:
  name: train_accuracy
  goal: maximize
parameters:
  learning_rate:
    min: 0.001
    max: 0.1
  epochs:
    values: [10, 20, 50, 100]
```

Run sweep:
```bash
wandb sweep sweep.yaml
wandb agent YOUR_SWEEP_ID
```

## References

- [Weights & Biases Documentation](https://docs.wandb.ai/)
- [SimpleDB Documentation](../database/README.md)
- [Cloud Development Guide](../../docs/cloud_development.md)
