#!/usr/bin/env python3
"""
Image Converter for 4.26" e-Paper Display
Converts standard image formats to raw binary format for e-Paper display
"""

from PIL import Image
import sys
import os

def convert_image_to_epaper(input_path, output_path, width=800, height=480):
    """
    Convert image to e-Paper binary format
    
    Args:
        input_path: Path to input image file
        output_path: Path to output binary file
        width: Display width (default: 800)
        height: Display height (default: 480)
    """
    try:
        # Open and process image
        print(f"Opening image: {input_path}")
        img = Image.open(input_path)
        
        # Resize to display resolution
        print(f"Resizing to {width}x{height}")
        img = img.resize((width, height), Image.LANCZOS)
        
        # Convert to 1-bit black and white
        print("Converting to monochrome...")
        img = img.convert('1')
        
        # Convert to binary format
        print("Generating binary data...")
        pixels = img.load()
        
        with open(output_path, 'wb') as f:
            byte_count = 0
            for y in range(height):
                for x in range(0, width, 8):
                    byte = 0
                    for bit in range(8):
                        if x + bit < width:
                            # White pixel = 1, Black pixel = 0
                            if pixels[x + bit, y]:
                                byte |= (0x80 >> bit)
                    f.write(bytes([byte]))
                    byte_count += 1
        
        print(f"✓ Conversion successful!")
        print(f"  Output file: {output_path}")
        print(f"  File size: {byte_count} bytes")
        print(f"  Expected size: {width * height // 8} bytes")
        
        return True
        
    except Exception as e:
        print(f"✗ Error: {e}")
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python image_converter.py <input_image> [output_file]")
        print("\nExample:")
        print("  python image_converter.py photo.jpg image.bin")
        print("\nSupported formats: PNG, JPG, BMP, GIF, etc.")
        sys.exit(1)
    
    input_path = sys.argv[1]
    
    if not os.path.exists(input_path):
        print(f"Error: Input file '{input_path}' not found")
        sys.exit(1)
    
    # Generate output filename
    if len(sys.argv) >= 3:
        output_path = sys.argv[2]
    else:
        base_name = os.path.splitext(input_path)[0]
        output_path = f"{base_name}_epaper.bin"
    
    # Convert image
    success = convert_image_to_epaper(input_path, output_path)
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
