#!/bin/bash

# Define source and target directories

DIRECTORY_TO_LINK="utilities"

SOURCE_DIR="./shared/c/$DIRECTORY_TO_LINK"
TARGET_DIR_LIST=("./chronomark-edition/src/c" "./standard-edition/src/c")

# Create target directories if they don't exist
for TARGET_DIR in "${TARGET_DIR_LIST[@]}"; do
  mkdir -p "$TARGET_DIR"

  # Create the symlink inside the target directory
  ln -sfn "$(realpath "$SOURCE_DIR")" "$TARGET_DIR/$DIRECTORY_TO_LINK"

  echo "Symlink created: $TARGET_DIR/$DIRECTORY_TO_LINK -> $(realpath "$SOURCE_DIR")"
done