using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Target : MonoBehaviour
{
  public bool clicked = false;
  public AudioClip hitSound;

  // Start is called before the first frame update
  void Start() {
    clicked = false;
    hitSound = Resources.Load<AudioClip>("Audio/hit_1");
  }

  // Update is called once per frame
  void Update() {
    
  }

  void OnMouseDown() {
    // int se = Random.Range(1, totalSEs + 1);
    // string filename = "Audio/hit_" + se.ToString();
    // hitSound = Resources.Load<AudioClip>(filename);

    clicked = true;
    Destroy(gameObject);
  }
}