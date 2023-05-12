import { Injectable } from '@angular/core';

@Injectable({
  providedIn: 'root',
})
export class PetService {
  constructor() {}

  getPetPosition() {
    return { lat: 41.8941, lng: 12.495, timestamp: Date.now() - 10000 };
  }
}
