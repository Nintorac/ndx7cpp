# Model Files

This directory should contain the trained PyTorch model file.

## Required File

- `dx7_vae_model.pt` - The trained DX7 VAE model from the Python project

## How to Generate the Model

1. Train the model using the Python code in the parent directory:
   ```bash
   cd ../projects/dx7_vae
   python experiment.py
   ```

2. Export the trained model to TorchScript format:
   ```python
   import torch
   from agoge import InferenceWorker
   
   # Load your trained model
   model = InferenceWorker('hasty-copper-dogfish', 'dx7-vae', with_data=False).model
   
   # Convert to TorchScript
   scripted_model = torch.jit.script(model)
   
   # Save the scripted model
   scripted_model.save('dx7_vae_model.pt')
   ```

3. Copy the `dx7_vae_model.pt` file to this directory.

The model will be embedded into the binary during the build process.


## Generate from the docker image

1. Run the image

```bash
docker run -it --rm -v ./models/:/models <INSERT_IMAGE_NAME> bash
```


2. In the image run the following

```bash
#!/bin/bash

# Echo the Python script into a file
cat > convert_model_to_torchscript.py << 'EOF'
import torch
from agoge import InferenceWorker

# Load your trained model
model = InferenceWorker('hasty-copper-dogfish', 'dx7-vae', with_data=False).model

# Set model to evaluation mode
model.eval()

# Create a dummy input tensor with shape (1, 8)
dummy_input = torch.randn(1, 8)

# Test the original model to make sure it works
print("Testing model inference...")
with torch.no_grad():
    p_x = model.generate(dummy_input)
    sample = p_x.logits.argmax(-1)
    print(f"Model output shape: {sample.shape}")
    print("Model inference successful!")

# Create a wrapper function that returns only tensors
class ModelWrapper(torch.nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model
    
    def forward(self, x):
        # Call the generate method and extract only the logits tensor
        p_x = self.model.generate(x)
        return p_x.logits

# Wrap the model
wrapped_model = ModelWrapper(model)

# Test the wrapped model
print("Testing wrapped model...")
with torch.no_grad():
    logits = wrapped_model(dummy_input)
    sample = logits.argmax(-1)
    print(f"Wrapped model output shape: {sample.shape}")
    print("Wrapped model test successful!")

# Convert to TorchScript using trace
print("Converting to TorchScript...")
traced_model = torch.jit.trace(wrapped_model, dummy_input)

# Save the traced model
traced_model.save('dx7_vae_model.pt')

print("Model successfully traced and saved as 'dx7_vae_model.pt'")

# Test the traced model
print("Testing traced model...")
loaded_model = torch.jit.load('dx7_vae_model.pt')
with torch.no_grad():
    traced_logits = loaded_model(dummy_input)
    traced_sample = traced_logits.argmax(-1)
    print(f"Traced model output shape: {traced_sample.shape}")
    print("Traced model test successful!")
    
    # Verify outputs match
    original_logits = wrapped_model(dummy_input)
    if torch.allclose(traced_logits, original_logits, atol=1e-5):
        print("✓ Traced model outputs match original!")
    else:
        print("⚠ Warning: Traced model outputs differ from original")

# Test with different input sizes to check generalization
print("\nTesting different input sizes...")
test_inputs = [
    torch.randn(16, 8),  # Half batch
    torch.randn(64, 8),  # Double batch
    torch.randn(1, 8),   # Single sample
    torch.randn(8, 8),   # Quarter batch
]

for i, test_input in enumerate(test_inputs):
    try:
        with torch.no_grad():
            output = loaded_model(test_input)
            print(f"✓ Test {i+1}: Works with shape {test_input.shape} → output shape {output.shape}")
    except Exception as e:
        print(f"✗ Test {i+1}: Fails with shape {test_input.shape}: {e}")

print("\nTracing complete! Check the test results above to see if your model generalizes to different batch sizes.")
EOF

# Run the Python script
python convert_model_to_torchscript.py
```

3. In a seperate terminal run the following, swap out the image name with the actual one

```bash
docker cp beautiful_hellman:/opt/dx7_vae_model.pt .
```