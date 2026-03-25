#!/bin/bash

# Define source and target directories

DIRECTORIES_TO_LINK=("src/c/utilities" "resources/weather" "resources/splash_logos")

TARGET_DIR_LIST=("./chronomark-edition/" "./standard-edition/")


for DIRECTORY_TO_LINK in "${DIRECTORIES_TO_LINK[@]}"; do
  SOURCE_DIR="./shared/$DIRECTORY_TO_LINK"

  if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Source directory '$SOURCE_DIR' does not exist."
    exit 1
  fi

  # Create target directories and symlinks for each target directory
  for TARGET_DIR in "${TARGET_DIR_LIST[@]}"; do
    mkdir -p "$TARGET_DIR"
    ln -sfn "$(realpath "$SOURCE_DIR")" "$TARGET_DIR/$DIRECTORY_TO_LINK"
    echo "Symlink created: $TARGET_DIR/$DIRECTORY_TO_LINK -> $(realpath "$SOURCE_DIR")"
  done
done